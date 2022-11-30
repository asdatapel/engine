#pragma once

#include "containers/static_stack.hpp"
#include "font.hpp"
#include "gpu/vulkan/vulkan.hpp"
#include "math/math.hpp"
#include "types.hpp"

namespace Dui
{

struct Vertex {
  Vec2f pos;
  Vec2f uv;
  Color color;
  f32 texture_blend_factor;
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

  Buffer vb;

  StaticStack<Rect, 1024> scissors;
  void push_scissor(Rect rect) { scissors.push_back(rect); }
  void pop_scissor() { scissors.pop(); }

  VkDescriptorSet desc_set;
};

DrawList main_dl;
DrawList forground_dl;
Font font;

void init_draw_list(DrawList *dl, Pipeline pipeline)
{
  dl->verts      = (Vertex *)malloc(sizeof(Vertex) * 1024 * 1024);
  dl->draw_calls = (DrawCall *)malloc(sizeof(DrawCall) * 1024 * 1024);

  dl->vb = create_vertex_buffer(MB);

  font =
      load_font("resources/fonts/OpenSans-Regular.ttf", 21, &system_allocator);

  ImageBuffer image      = create_image(font.atlas.width, font.atlas.height);
  VkImageView image_view = create_image_view(image);
  VkSampler sampler      = create_sampler();
  upload_image(image, font.atlas);

  dl->desc_set = create_descriptor_set(pipeline);
  write_sampler(dl->desc_set, image_view, sampler);
  vkDeviceWaitIdle(vk.device);
}

void clear_draw_list(DrawList *dl)
{
  dl->vert_count      = 0;
  dl->draw_call_count = 0;
}

void draw_draw_list(Pipeline pipeline, DrawList *dl, Vec2f canvas_size)
{
  upload_buffer_staged(dl->vb, dl->verts, dl->vert_count * sizeof(Vertex));
  bind_descriptor_set(pipeline, dl->desc_set);

  push_constant(pipeline, &canvas_size, sizeof(Vec2f));
  for (i32 i = 0; i < dl->draw_call_count; i++) {
    DrawCall call = dl->draw_calls[i];

    set_scissor(call.scissor);
    draw_vertex_buffer(dl->vb, call.vert_offset, call.tri_count * 3);
  }
}

void push_vert(DrawList *dl, Vec2f pos, Vec2f uv, Color color,
               f32 texture_blend_factor = 0.f)
{
  dl->verts[dl->vert_count++] = {
      pos, uv, color, texture_blend_factor};
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

void push_rect(DrawList *dl, Rect rect, Color color)
{
  push_vert(dl, {rect.x, rect.y}, {}, color);
  push_vert(dl, {rect.x + rect.width, rect.y}, {}, color);
  push_vert(dl, {rect.x + rect.width, rect.y + rect.height}, {}, color);

  push_vert(dl, {rect.x, rect.y}, {}, color);
  push_vert(dl, {rect.x + rect.width, rect.y + rect.height}, {}, color);
  push_vert(dl, {rect.x, rect.y + rect.height}, {}, color);

  push_draw_call(dl, 2);
}

void push_text(DrawList *dl, String text, Vec2f pos, Color color, f32 height)
{
  f32 baseline     = font.baseline;
  f32 resize_ratio = height / font.font_size_px;
  for (int i = 0; i < text.size; i++) {
    Character c     = font.characters[text.data[i]];
    Rect shape_rect = {
        pos.x + c.shape.x * resize_ratio, pos.y + (c.shape.y) * resize_ratio,
        c.shape.width * resize_ratio, c.shape.height * resize_ratio};
    pos.x += c.advance * resize_ratio;

    shape_rect.x      = floorf(shape_rect.x);
    shape_rect.y      = floorf(shape_rect.y);

    push_vert(dl, {shape_rect.x, shape_rect.y}, {c.uv.x, c.uv.y + c.uv.height},
              color, color.a);
    push_vert(dl, {shape_rect.x + shape_rect.width, shape_rect.y},
              {c.uv.x + c.uv.width, c.uv.y + c.uv.height}, color, color.a);
    push_vert(
        dl, {shape_rect.x + shape_rect.width, shape_rect.y + shape_rect.height},
        {c.uv.x + c.uv.width, c.uv.y}, color, color.a);

    push_vert(dl, {shape_rect.x, shape_rect.y}, {c.uv.x, c.uv.y + c.uv.height},
              color, color.a);
    push_vert(
        dl, {shape_rect.x + shape_rect.width, shape_rect.y + shape_rect.height},
        {c.uv.x + c.uv.width, c.uv.y}, color, color.a);
    push_vert(dl, {shape_rect.x, shape_rect.y + shape_rect.height},
              {c.uv.x, c.uv.y}, color, color.a);
    push_draw_call(dl, 2);
  }
};

void push_text_centered(DrawList *dl, String text, Vec2f pos, Vec2b center,
                        Color color, f32 height)
{
  f32 resize_ratio = height / font.font_size_px;
  f32 line_width   = get_text_width(font, text, resize_ratio);

  Vec2f centered_pos;
  centered_pos.x = center.x ? pos.x - line_width / 2 : pos.x;
  centered_pos.y =
      center.y ? pos.y + (font.ascent + font.descent) / 2.f : pos.y;

  push_text(dl, text, centered_pos, color, height);
}

// TODO allow pushing a temporary drawcall then can be updated later.
// Useful for grouping controls with a background
DrawCall *push_temp();

}  // namespace Dui