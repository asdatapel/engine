#pragma once

#include "containers/static_stack.hpp"
#include "font.hpp"
#include "gpu/vulkan/vulkan.hpp"
#include "math/math.hpp"
#include "types.hpp"

namespace Dui
{

const u32 CORNERS[] = {
    0x00000000,
    0x00010000,
    0x00020000,
    0x00030000,
};

enum struct PrimitiveIds : u32 {
  RECT         = 1 << 18,
  ROUNDED_RECT = 2 << 18,
  BITMAP_GLYPH = 3 << 18,
};

struct RectPrimitive {
  Rect rect;
};

struct RoundedRectPrimitive {
  Rect dimensions;
  u32 clip_rect_idx;
  u32 color;
  f32 corner_radius;
  f32 pad;
};

struct BitmapGlyphPrimitive {
  Rect dimensions;
  Vec4f uv_bounds;
  u32 clip_rect_idx;
  u32 color;
  Vec2f pad;
};

struct Primitives {
  RectPrimitive clip_rects[1024];
  RoundedRectPrimitive rounded_rects[1024];
  BitmapGlyphPrimitive bitmap_glyphs[1024];
};

struct GlobalDrawListData {
  u64 frame = 0;

  ImageBuffer image;
  VkImageView image_view;
  VkSampler sampler;
  VkDescriptorSet desc_set;

  Buffer primitive_buffer;

  Font font;

  Primitives primitives;
  i32 clip_rects_count    = 0;
  i32 rounded_rects_count = 0;
  i32 bitmap_glyphs_count = 0;

  StaticStack<Rect, 1024> scissors;
  StaticStack<u32, 1024> scissor_idxs;
};

struct DrawCall {
  i32 vert_offset;
  i32 tri_count;

  Rect scissor;
  // Texture texture;
};

struct DrawList {
  // TODO: we can just have a single index buffer in the gdld
  u32 *verts     = nullptr;
  i32 vert_count = 0;

  DrawCall *draw_calls = nullptr;
  i32 draw_call_count  = 0;

  Buffer index_buffer;
};

void init_draw_system(GlobalDrawListData *gdld, Device *device,
                      Pipeline pipeline)
{
  // TODO: create pipeline here
  gdld->desc_set = create_descriptor_set(device, pipeline);

  gdld->font =
      load_font("resources/fonts/OpenSans-Regular.ttf", 21, &system_allocator);
  gdld->image =
      create_image(device, gdld->font.atlas.width, gdld->font.atlas.height);
  gdld->image_view = create_image_view(device, gdld->image);
  gdld->sampler    = create_sampler(device);
  upload_image(device, gdld->image, gdld->font.atlas);
  write_sampler(device, gdld->desc_set, gdld->image_view, gdld->sampler, 0);

  gdld->primitive_buffer = create_storage_buffer(device, MB);
  bind_storage_buffer(device, gdld->desc_set, gdld->primitive_buffer, 1);
}

void push_scissor(GlobalDrawListData *gdld, Rect rect)
{
  gdld->primitives.clip_rects[gdld->clip_rects_count++] = {rect};
  gdld->scissor_idxs.push_back(gdld->clip_rects_count - 1);
  gdld->scissors.push_back(rect);
}
void pop_scissor(GlobalDrawListData *gdld)
{
  gdld->scissor_idxs.pop();
  gdld->scissors.pop();
}

void draw_system_start_frame(GlobalDrawListData *gdld)
{
  gdld->clip_rects_count    = 0;
  gdld->rounded_rects_count = 0;
  gdld->bitmap_glyphs_count = 0;

  gdld->scissor_idxs.clear();
  gdld->scissors.clear();

  push_scissor(gdld, {0, 0, 100000, 100000});
}

void init_draw_list(DrawList *dl, Device *device)
{
  dl->draw_calls   = (DrawCall *)malloc(sizeof(DrawCall) * 1024 * 1024);
  dl->verts        = (u32 *)malloc(sizeof(u32) * 1024 * 1024);
  dl->index_buffer = create_index_buffer(device, MB);
}

void clear_draw_list(DrawList *dl)
{
  dl->vert_count      = 0;
  dl->draw_call_count = 0;
}

void draw_draw_list(DrawList *dl, GlobalDrawListData *gdld, Device *device,
                    Pipeline pipeline, Vec2f canvas_size, u64 frame)
{
  if (gdld->frame != frame) {
    gdld->frame = frame;

    upload_buffer_staged(device, gdld->primitive_buffer, &gdld->primitives,
                         sizeof(gdld->primitives));
  }

  bind_descriptor_set(device, pipeline, gdld->desc_set);

  upload_buffer_staged(device, dl->index_buffer, dl->verts,
                       dl->vert_count * sizeof(u32));
  push_constant(device, pipeline, &canvas_size, sizeof(Vec2f));
  for (i32 i = 0; i < dl->draw_call_count; i++) {
    DrawCall call = dl->draw_calls[i];

    //   set_scissor(device, call.scissor);
    draw_indexed(device, dl->index_buffer, call.vert_offset,
                 call.tri_count * 3);
  }
}

void push_draw_call(DrawList *dl, i32 tri_count)
{
  if (dl->draw_call_count > 0) {
    dl->draw_calls[dl->draw_call_count - 1].tri_count += tri_count;
    return;
  }

  DrawCall dc = {dl->vert_count - (tri_count * 3), tri_count};
  dl->draw_calls[dl->draw_call_count] = dc;
  dl->draw_call_count++;
}

u32 push_primitive_rounded_rect(GlobalDrawListData *gdld, Rect rect,
                                Color color, f32 corner_radius)
{
  auto color_to_int = [](Color c) {
    u32 r = (u32)(c.r * 255) & 0xFF;
    u32 g = (u32)(c.g * 255) & 0xFF;
    u32 b = (u32)(c.b * 255) & 0xFF;
    u32 a = (u32)(c.a * 255) & 0xFF;
    return (r << 24) | (g << 16) | (b << 8) | (a);
  };
  gdld->primitives.rounded_rects[gdld->rounded_rects_count++] = {
      rect, gdld->scissor_idxs.top(), color_to_int(color), corner_radius};

  return gdld->rounded_rects_count - 1;
}

void push_rounded_rect(DrawList *dl, GlobalDrawListData *gdld, Rect rect,
                       f32 corner_radius, Color color)
{
  auto push_rect_vert = [](DrawList *dl, u32 primitive_index, u8 corner) {
    dl->verts[dl->vert_count++] = {(u32)PrimitiveIds::ROUNDED_RECT |
                                   CORNERS[corner] | primitive_index};
  };

  if (!overlaps(rect, gdld->scissors.top())) {
    return;
  }

  u32 primitive_idx =
      push_primitive_rounded_rect(gdld, rect, color, corner_radius);

  push_rect_vert(dl, primitive_idx, 0);
  push_rect_vert(dl, primitive_idx, 1);
  push_rect_vert(dl, primitive_idx, 2);
  push_rect_vert(dl, primitive_idx, 1);
  push_rect_vert(dl, primitive_idx, 3);
  push_rect_vert(dl, primitive_idx, 2);

  push_draw_call(dl, 2);
}

u32 push_primitive_bitmap_glyph(GlobalDrawListData *gdld, Rect rect,
                                Vec4f uv_bounds, Color color)
{
  auto color_to_int = [](Color c) {
    u32 r = (u32)(c.r * 255) & 0xFF;
    u32 g = (u32)(c.g * 255) & 0xFF;
    u32 b = (u32)(c.b * 255) & 0xFF;
    u32 a = (u32)(c.a * 255) & 0xFF;
    return (r << 24) | (g << 16) | (b << 8) | (a);
  };
  gdld->primitives.bitmap_glyphs[gdld->bitmap_glyphs_count++] = {
      rect, uv_bounds, gdld->scissor_idxs.top(), color_to_int(color)};

  return gdld->bitmap_glyphs_count - 1;
}

void push_bitmap_glyph(DrawList *dl, GlobalDrawListData *gdld, Rect rect,
                       Vec4f uv_bounds, Color color)
{
  auto push_glyph_vert = [](DrawList *dl, u32 primitive_index, u8 corner) {
    dl->verts[dl->vert_count++] = {(u32)PrimitiveIds::BITMAP_GLYPH |
                                   CORNERS[corner] | primitive_index};
  };

  if (!overlaps(rect, gdld->scissors.top())) {
    return;
  }

  u32 primitive_idx = push_primitive_bitmap_glyph(gdld, rect, uv_bounds, color);

  push_glyph_vert(dl, primitive_idx, 0);
  push_glyph_vert(dl, primitive_idx, 1);
  push_glyph_vert(dl, primitive_idx, 2);
  push_glyph_vert(dl, primitive_idx, 1);
  push_glyph_vert(dl, primitive_idx, 3);
  push_glyph_vert(dl, primitive_idx, 2);

  push_draw_call(dl, 2);
}

void push_rect(DrawList *dl, GlobalDrawListData *gdld, Rect rect, Color color)
{
  push_rounded_rect(dl, gdld, rect, 0, color);
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

    Vec4f uv_bounds = {c.uv.x, c.uv.y + c.uv.height, c.uv.x + c.uv.width,
                       c.uv.y};
    push_bitmap_glyph(dl, gdld, shape_rect, uv_bounds, color);
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