#pragma once

#include "containers/static_stack.hpp"
#include "font.hpp"
#include "font/vector_font.hpp"
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
  TEXTURE_RECT = 3 << 18,
  BITMAP_GLYPH = 4 << 18,
  VECTOR_GLYPH = 5 << 18,
};

struct RectPrimitive {
  Rect rect;
};

struct RoundedRectPrimitive {
  Rect dimensions;
  u32 clip_rect_idx;
  u32 color;
  f32 corner_radius;
  u32 corner_mask;
};

struct TextureRectPrimitive {
  Rect dimensions;
  Vec4f uv_bounds;
  u32 texture_idx;
  u32 clip_rect_idx;
  Vec2f pad;
};

struct BitmapGlyphPrimitive {
  Rect dimensions;
  Vec4f uv_bounds;
  u32 clip_rect_idx;
  u32 color;
  Vec2f pad;
};

struct ConicCurvePrimitive {
  Vec2f p0, p1, p2;
  Vec2f pad;
};

struct VectorGlyphPrimitive {
  Rect dimensions;
  u32 curve_start_idx;
  u32 curve_count;
  u32 color;
  u32 clip_rect_idx;
};

struct Primitives {
  RectPrimitive clip_rects[1024];
  RoundedRectPrimitive rounded_rects[1024];
  BitmapGlyphPrimitive bitmap_glyphs[1024];
  TextureRectPrimitive texture_rects[1024];
  VectorGlyphPrimitive vector_glyphs[1024];
  ConicCurvePrimitive conic_curves[2048];
};

struct GlobalDrawListData {
  u64 frame = 0;

  ImageBuffer image;
  VkImageView image_view;
  VkSampler sampler;
  VkDescriptorSet desc_set;

  Buffer primitive_buffer;

  Font font;
  VectorFont vfont;

  u32 texture_count = 1;

  Primitives primitives;
  i32 clip_rects_count    = 0;
  i32 rounded_rects_count = 0;
  i32 texture_rects_count = 0;
  i32 bitmap_glyphs_count = 0;
  i32 conic_curves_count  = 0;
  i32 vector_glyphs_count = 0;

  StaticStack<Rect, 1024> scissors;
  StaticStack<u32, 1024> scissor_idxs;
};

struct DrawCall {
  i32 vert_offset;
  i32 tri_count;

  Rect scissor;
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

  gdld->vfont = create_font();
  for (i32 i = 0; i < gdld->vfont.curves.size; i++) {
    gdld->primitives.conic_curves[gdld->conic_curves_count++] = {
        gdld->vfont.curves[i].p0, gdld->vfont.curves[i].p1,
        gdld->vfont.curves[i].p2};
  }

  gdld->primitive_buffer = create_storage_buffer(device, MB);
  bind_storage_buffer(device, gdld->desc_set, gdld->primitive_buffer, 1);
}

void push_scissor(GlobalDrawListData *gdld, Rect rect)
{
  auto to_bounds = [](Rect r) {
    return Vec4f{r.x, r.y, r.x + r.width, r.y + r.height};
  };
  if (gdld->scissors.count > 0) {
    Vec4f rect_bounds   = to_bounds(rect);
    Vec4f parent_bounds = to_bounds(gdld->scissors.top());

    rect_bounds.x = fmaxf(rect_bounds.x, parent_bounds.x);
    rect_bounds.y = fmaxf(rect_bounds.y, parent_bounds.y);
    rect_bounds.z = fminf(rect_bounds.z, parent_bounds.z);
    rect_bounds.w = fminf(rect_bounds.w, parent_bounds.w);

    rect = {rect_bounds.x, rect_bounds.y, rect_bounds.z - rect_bounds.x,
            rect_bounds.w - rect_bounds.y};
  }

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
  gdld->texture_rects_count = 0;
  gdld->bitmap_glyphs_count = 0;
  gdld->vector_glyphs_count = 0;

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

u32 push_texture(Device *device, GlobalDrawListData *gdld,
                 VkImageView image_view, VkSampler sampler)
{
  bind_sampler(device, gdld->desc_set, image_view, sampler, 0,
               gdld->texture_count++);
  return gdld->texture_count - 1;
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

enum CornerMask : u32 {
  TOP_LEFT     = 0b0001,
  TOP_RIGHT    = 0b0010,
  BOTTOM_LEFT  = 0b0100,
  BOTTOM_RIGHT = 0b1000,
  TOP          = TOP_LEFT | TOP_RIGHT,
  BOTTOM       = BOTTOM_LEFT | BOTTOM_RIGHT,
  ALL          = TOP | BOTTOM,
};
u32 push_primitive_rounded_rect(GlobalDrawListData *gdld, Rect rect,
                                Color color, f32 corner_radius, u32 corner_mask)
{
  gdld->primitives.rounded_rects[gdld->rounded_rects_count++] = {
      rect, gdld->scissor_idxs.top(), color_to_int(color), corner_radius,
      corner_mask};

  return gdld->rounded_rects_count - 1;
}

void push_rounded_rect(DrawList *dl, GlobalDrawListData *gdld, Rect rect,
                       f32 corner_radius, Color color,
                       u32 corner_mask = CornerMask::ALL)
{
  auto push_rect_vert = [](DrawList *dl, u32 primitive_index, u8 corner) {
    dl->verts[dl->vert_count++] = {(u32)PrimitiveIds::ROUNDED_RECT |
                                   CORNERS[corner] | primitive_index};
  };

  if (!overlaps(rect, gdld->scissors.top())) {
    return;
  }

  u32 primitive_idx = push_primitive_rounded_rect(gdld, rect, color,
                                                  corner_radius, corner_mask);

  push_rect_vert(dl, primitive_idx, 0);
  push_rect_vert(dl, primitive_idx, 1);
  push_rect_vert(dl, primitive_idx, 2);
  push_rect_vert(dl, primitive_idx, 1);
  push_rect_vert(dl, primitive_idx, 3);
  push_rect_vert(dl, primitive_idx, 2);

  push_draw_call(dl, 2);
}

void push_rect(DrawList *dl, GlobalDrawListData *gdld, Rect rect, Color color)
{
  push_rounded_rect(dl, gdld, rect, 0, color);
}

u32 push_primitive_bitmap_glyph(GlobalDrawListData *gdld, Rect rect,
                                Vec4f uv_bounds, Color color)
{
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

u32 push_primitive_vector_glyph(GlobalDrawListData *gdld, Rect rect,
                                Glyph glyph, Color color)
{
  gdld->primitives.vector_glyphs[gdld->vector_glyphs_count++] = {
      rect, glyph.curve_start_idx, glyph.curve_count, color_to_int(color),
      gdld->scissor_idxs.top()};

  return gdld->vector_glyphs_count - 1;
}

void push_vector_glyph(DrawList *dl, GlobalDrawListData *gdld, Rect rect,
                       Glyph glyph, Color color)
{
  auto push_glyph_vert = [](DrawList *dl, u32 primitive_index, u8 corner) {
    dl->verts[dl->vert_count++] = {(u32)PrimitiveIds::VECTOR_GLYPH |
                                   CORNERS[corner] | primitive_index};
  };

  if (!overlaps(rect, gdld->scissors.top())) {
    return;
  }

  u32 primitive_idx = push_primitive_vector_glyph(gdld, rect, glyph, color);

  push_glyph_vert(dl, primitive_idx, 0);
  push_glyph_vert(dl, primitive_idx, 1);
  push_glyph_vert(dl, primitive_idx, 2);
  push_glyph_vert(dl, primitive_idx, 1);
  push_glyph_vert(dl, primitive_idx, 3);
  push_glyph_vert(dl, primitive_idx, 2);

  push_draw_call(dl, 2);
}

void push_vector_text(DrawList *dl, GlobalDrawListData *gdld, String text,
                      Vec2f pos, Color color, f32 size)
{
  for (int i = 0; i < text.size; i++) {
    Glyph g = gdld->vfont.glyphs[text.data[i]];

    f32 width       = size * g.size.x;
    f32 height      = size * g.size.y;
    Rect shape_rect = {
        pos.x + (size * g.bearing.x),
        pos.y + (size * gdld->vfont.ascent) - (size * g.bearing.y), width,
        height};
    pos.x += size * g.advance;

    push_vector_glyph(dl, gdld, shape_rect, gdld->vfont.glyphs[text.data[i]],
                      color);
  }
};

void push_vector_text_centered(DrawList *dl, GlobalDrawListData *gdld,
                               String text, Vec2f pos, Color color, f32 size,
                               Vec2b center)
{
  if (center.x) {
    f32 text_width = 0;
    for (int i = 0; i < text.size - 1; i++) {
      Glyph g = gdld->vfont.glyphs[text.data[i]];
      text_width += size * g.advance;
    }
    text_width += gdld->vfont.glyphs[text.data[text.size - 1]].size.y * size;

    pos.x -= text_width / 2;
  }
  if (center.y) pos.y -= size / 2;

  for (int i = 0; i < text.size; i++) {
    Glyph g = gdld->vfont.glyphs[text.data[i]];

    f32 width       = size * g.size.x;
    f32 height      = size * g.size.y;
    Rect shape_rect = {
        pos.x + (size * g.bearing.x),
        pos.y + (size * gdld->vfont.ascent) - (size * g.bearing.y), width,
        height};
    pos.x += size * g.advance;

    push_vector_glyph(dl, gdld, shape_rect, gdld->vfont.glyphs[text.data[i]],
                      color);
  }
};

void push_texture_rect(DrawList *dl, GlobalDrawListData *gdld, Rect rect,
                       Vec4f uv_bounds, u32 texture_id)
{
  auto push_primitive_texture_rect = [](GlobalDrawListData *gdld, Rect rect,
                                        Vec4f uv_bounds, u32 texture_id) {
    gdld->primitives.texture_rects[gdld->texture_rects_count++] = {
        rect, uv_bounds, texture_id, gdld->scissor_idxs.top()};

    return gdld->texture_rects_count - 1;
  };
  auto push_texture_rect_vert = [](DrawList *dl, u32 primitive_index,
                                   u8 corner) {
    dl->verts[dl->vert_count++] = {(u32)PrimitiveIds::TEXTURE_RECT |
                                   CORNERS[corner] | primitive_index};
  };

  if (!overlaps(rect, gdld->scissors.top())) {
    return;
  }

  u32 primitive_idx =
      push_primitive_texture_rect(gdld, rect, uv_bounds, texture_id);

  push_texture_rect_vert(dl, primitive_idx, 0);
  push_texture_rect_vert(dl, primitive_idx, 1);
  push_texture_rect_vert(dl, primitive_idx, 2);
  push_texture_rect_vert(dl, primitive_idx, 1);
  push_texture_rect_vert(dl, primitive_idx, 3);
  push_texture_rect_vert(dl, primitive_idx, 2);

  push_draw_call(dl, 2);
}

// TODO allow pushing a temporary drawcall then can be updated later.
// Useful for grouping controls with a background
DrawCall *push_temp();

}  // namespace Dui