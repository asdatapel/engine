#pragma once

#include "containers/static_stack.hpp"
#include "font.hpp"
#include "gpu/vulkan/vulkan.hpp"
#include "math/math.hpp"
#include "types.hpp"

namespace Dui
{

struct GlobalDrawListData {
  ImageBuffer image;
  VkImageView image_view;
  VkSampler sampler;
  VkDescriptorSet desc_set;

  Font font;
};

struct Vertex {
  Vec2f pos;
  Vec2f uv;
  Color color;
  f32 texture_blend_factor;
  Vec2f rect_corner;
  Vec2f rect_center;
};

struct DrawCall {
  i32 vert_offset;
  i32 tri_count;

  Rect scissor;
  // Texture texture;
};

struct DrawList {
  Vertex *verts        = nullptr;
  DrawCall *draw_calls = nullptr;
  i32 vert_count       = 0;
  i32 draw_call_count  = 0;

  StaticStack<Rect, 1024> scissors;
  void push_scissor(Rect rect) { scissors.push_back(rect); }
  void pop_scissor() { scissors.pop(); }

  Buffer vb;
};

GlobalDrawListData init_draw_system(Device *device, Pipeline pipeline)
{
  GlobalDrawListData gdld;

  // TODO: create pipeline here

  gdld.font =
      load_font("resources/fonts/OpenSans-Regular.ttf", 21, &system_allocator);

  gdld.image =
      create_image(device, gdld.font.atlas.width, gdld.font.atlas.height);
  gdld.image_view = create_image_view(device, gdld.image);
  gdld.sampler    = create_sampler(device);
  upload_image(device, gdld.image, gdld.font.atlas);

  gdld.desc_set = create_descriptor_set(device, pipeline);
  write_sampler(device, gdld.desc_set, gdld.image_view, gdld.sampler);

  return gdld;
}

void init_draw_list(DrawList *dl, Device *device)
{
  dl->verts      = (Vertex *)malloc(sizeof(Vertex) * 1024 * 1024);
  dl->draw_calls = (DrawCall *)malloc(sizeof(DrawCall) * 1024 * 1024);

  dl->vb = create_vertex_buffer(device, MB);
}

void clear_draw_list(DrawList *dl)
{
  dl->vert_count      = 0;
  dl->draw_call_count = 0;
}

void draw_draw_list(DrawList *dl, GlobalDrawListData *gdld, Device *device,
                    Pipeline pipeline, Vec2f canvas_size)
{
  upload_buffer_staged(device, dl->vb, dl->verts,
                       dl->vert_count * sizeof(Vertex));
  bind_descriptor_set(device, pipeline, gdld->desc_set);

  push_constant(device, pipeline, &canvas_size, sizeof(Vec2f));
  for (i32 i = 0; i < dl->draw_call_count; i++) {
    DrawCall call = dl->draw_calls[i];

    set_scissor(device, call.scissor);
    draw_vertex_buffer(device, dl->vb, call.vert_offset, call.tri_count * 3);
  }
}

void push_vert(DrawList *dl, Vec2f pos, Vec2f uv, Color color,
               Vec2f rect_corner, Vec2f rect_center,
               f32 texture_blend_factor = 0.f)
{
  dl->verts[dl->vert_count++] = {
      pos, uv, color, texture_blend_factor, rect_corner, rect_center};
}

void push_draw_call(DrawList *dl, i32 tri_count)
{
  Rect current_scissor_rect = (dl->scissors.count == 0)
                                  ? Rect{0, 0, 10000000, 10000000}
                                  : dl->scissors.top();

  if (dl->draw_call_count > 0 &&
      dl->draw_calls[dl->draw_call_count - 1].scissor == current_scissor_rect) {
    dl->draw_calls[dl->draw_call_count - 1].tri_count += tri_count;
    return;
  }

  DrawCall dc = {dl->vert_count - (tri_count * 3), tri_count};
  dc.scissor  = current_scissor_rect;

  dl->draw_calls[dl->draw_call_count] = dc;
  dl->draw_call_count++;
}

void push_tri(DrawList *dl, Vec2f p0, Vec2f p1, Vec2f p2, Color color)
{
  push_vert(dl, p0, {}, color, {0, 0}, {0, 0});
  push_vert(dl, p1, {}, color, {0, 0}, {0, 0});
  push_vert(dl, p2, {}, color, {0, 0}, {0, 0});

  push_draw_call(dl, 1);
}

void push_quad(DrawList *dl, Vec2f p0, Vec2f p1, Vec2f p2, Vec2f p3,
               Color color)
{
  push_vert(dl, p0, {}, color, {0, 0}, {0, 0});
  push_vert(dl, p2, {}, color, {0, 0}, {0, 0});
  push_vert(dl, p1, {}, color, {0, 0}, {0, 0});

  push_vert(dl, p0, {}, color, {0, 0}, {0, 0});
  push_vert(dl, p3, {}, color, {0, 0}, {0, 0});
  push_vert(dl, p2, {}, color, {0, 0}, {0, 0});

  push_draw_call(dl, 2);
}

void push_rect(DrawList *dl, Rect rect, Color color)
{
  Vec2f corner = rect.span() / Vec2f(2.f, 2.f);
  Vec2f center = rect.center();

  push_vert(dl, {rect.x, rect.y}, {}, color, corner, center);
  push_vert(dl, {rect.x + rect.width, rect.y}, {}, color, corner, center);
  push_vert(dl, {rect.x + rect.width, rect.y + rect.height}, {}, color, corner,
            center);

  push_vert(dl, {rect.x, rect.y}, {}, color, corner, center);
  push_vert(dl, {rect.x + rect.width, rect.y + rect.height}, {}, color, corner,
            center);
  push_vert(dl, {rect.x, rect.y + rect.height}, {}, color, corner, center);

  push_draw_call(dl, 2);
}

void push_text(DrawList *dl, GlobalDrawListData *gdld, String text, Vec2f pos,
               Color color, f32 height)
{
  f32 baseline     = gdld->font.baseline;
  f32 resize_ratio = height / gdld->font.font_size_px;
  for (int i = 0; i < text.size; i++) {
    Character c     = gdld->font.characters[text.data[i]];
    Rect shape_rect = {
        pos.x + c.shape.x * resize_ratio, pos.y + (c.shape.y) * resize_ratio,
        c.shape.width * resize_ratio, c.shape.height * resize_ratio};
    pos.x += c.advance * resize_ratio;

    shape_rect.x = floorf(shape_rect.x);
    shape_rect.y = floorf(shape_rect.y);

    push_vert(dl, {shape_rect.x, shape_rect.y}, {c.uv.x, c.uv.y + c.uv.height},
              color, {0, 0}, {0, 0}, color.a);
    push_vert(dl, {shape_rect.x + shape_rect.width, shape_rect.y},
              {c.uv.x + c.uv.width, c.uv.y + c.uv.height}, color, {0, 0},
              {0, 0}, color.a);
    push_vert(
        dl, {shape_rect.x + shape_rect.width, shape_rect.y + shape_rect.height},
        {c.uv.x + c.uv.width, c.uv.y}, color, {0, 0}, {0, 0}, color.a);

    push_vert(dl, {shape_rect.x, shape_rect.y}, {c.uv.x, c.uv.y + c.uv.height},
              color, {0, 0}, {0, 0}, color.a);
    push_vert(
        dl, {shape_rect.x + shape_rect.width, shape_rect.y + shape_rect.height},
        {c.uv.x + c.uv.width, c.uv.y}, color, {0, 0}, {0, 0}, color.a);
    push_vert(dl, {shape_rect.x, shape_rect.y + shape_rect.height},
              {c.uv.x, c.uv.y}, color, {0, 0}, {0, 0}, color.a);
    push_draw_call(dl, 2);
  }
};

void push_text_centered(DrawList *dl, GlobalDrawListData *gdld, String text,
                        Vec2f pos, Vec2b center, Color color, f32 height)
{
  f32 resize_ratio = height / gdld->font.font_size_px;
  f32 line_width   = get_text_width(gdld->font, text, resize_ratio);

  Vec2f centered_pos;
  centered_pos.x = center.x ? pos.x - line_width / 2 : pos.x;
  centered_pos.y =
      center.y ? pos.y + (gdld->font.ascent + gdld->font.descent) / 2.f : pos.y;

  push_text(dl, gdld, text, centered_pos, color, height);
}

// TODO allow pushing a temporary drawcall then can be updated later.
// Useful for grouping controls with a background
DrawCall *push_temp();

}  // namespace Dui