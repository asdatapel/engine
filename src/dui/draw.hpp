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
  LINE         = 6 << 18,
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

struct LinePrimitive {
  Vec2f a;
  Vec2f b;
  u32 color;
  u32 clip_rect_idx;
  Vec2f pad;
};

struct Primitives {
  RectPrimitive clip_rects[1024];
  RoundedRectPrimitive rounded_rects[1024];
  BitmapGlyphPrimitive bitmap_glyphs[1024];
  TextureRectPrimitive texture_rects[1024];
  VectorGlyphPrimitive vector_glyphs[1024];
  ConicCurvePrimitive conic_curves[4096];
  LinePrimitive lines[4096];
};

struct DrawCall {
  i32 vert_offset;
  i32 tri_count;

  i32 z;
};

struct DrawSettings {
  b8 force_scissor = false;
  u32 scissor_idx = 0;
}; 

struct DrawList {
  u64 frame = 0;

  Gpu::ImageBuffer image;
  VkImageView image_view;
  VkSampler sampler;
  VkDescriptorSet desc_set;

  Gpu::Buffer primitive_buffer;

  Font font;
  VectorFont vfont;
  VectorFont icon_font;

  u32 texture_count = 1;

  Primitives primitives;
  i32 clip_rects_count    = 0;
  i32 rounded_rects_count = 0;
  i32 texture_rects_count = 0;
  i32 bitmap_glyphs_count = 0;
  i32 conic_curves_count  = 0;
  i32 vector_glyphs_count = 0;
  i32 lines_count         = 0;

  StaticStack<Rect, 1024> scissors;
  StaticStack<u32, 1024> scissor_idxs;

  StaticStack<DrawSettings, 32> settings;

  u32 *verts     = nullptr;
  i32 vert_count = 0;
  Gpu::Buffer index_buffer;

  Array<DrawCall, 128> draw_calls;
  i32 max_z = 0;
};

void push_draw_settings(DrawList *dl, DrawSettings ds) {
  dl->settings.push_back(ds);
}
void pop_draw_settings(DrawList *dl) {
  dl->settings.pop();
}

u32 get_current_scissor_idx(DrawList *dl) {
  if (dl->settings.size > 0 && dl->settings.top().force_scissor) {
    return dl->settings.top().scissor_idx;
  }
  return dl->scissor_idxs.top(); 
}
Rect get_current_scissor(DrawList *dl) {
  if (dl->settings.size > 0 && dl->settings.top().force_scissor) {
    return dl->primitives.clip_rects[dl->settings.top().scissor_idx].rect;
  }
  return dl->scissors.top(); 
}
u32 push_scissor(DrawList *dl, Rect rect)
{
  auto to_bounds = [](Rect r) {
    return Vec4f{r.x, r.y, r.x + r.width, r.y + r.height};
  };
  if (dl->scissors.size > 0) {
    Vec4f rect_bounds   = to_bounds(rect);
    Vec4f parent_bounds = to_bounds(dl->scissors.top());

    rect_bounds.x = fmaxf(rect_bounds.x, parent_bounds.x);
    rect_bounds.y = fmaxf(rect_bounds.y, parent_bounds.y);
    rect_bounds.z = fminf(rect_bounds.z, parent_bounds.z);
    rect_bounds.w = fminf(rect_bounds.w, parent_bounds.w);

    rect = {rect_bounds.x, rect_bounds.y, rect_bounds.z - rect_bounds.x,
            rect_bounds.w - rect_bounds.y};
  }

  dl->primitives.clip_rects[dl->clip_rects_count++] = {rect};
  dl->scissor_idxs.push_back(dl->clip_rects_count - 1);
  dl->scissors.push_back(rect);

  return dl->clip_rects_count - 1;
}
void pop_scissor(DrawList *dl)
{
  dl->scissor_idxs.pop();
  dl->scissors.pop();
}
void push_base_scissor(DrawList *dl)
{
  dl->scissor_idxs.push_back(0);
  dl->scissors.push_back(dl->scissors[0]);
}

u32 push_texture(Gpu::Device *device, DrawList *dl, VkImageView image_view,
                 VkSampler sampler)
{
  bind_sampler(device, dl->desc_set, image_view, sampler, 0,
               dl->texture_count++);
  return dl->texture_count - 1;
}

void push_draw_call(DrawList *dl, i32 tri_count, i32 z)
{
  if (dl->draw_calls.size > 0) {
    DrawCall *last_draw_call = &dl->draw_calls[dl->draw_calls.size - 1];
    if (last_draw_call->z == z) {
      last_draw_call->tri_count += tri_count;
      return;
    }
  }

  DrawCall dc = {dl->vert_count - (tri_count * 3), tri_count, z};
  dl->draw_calls.push_back(dc);
  dl->max_z = std::max(dl->max_z, z);
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
u32 push_primitive_rounded_rect(DrawList *dl, Rect rect, Color color,
                                f32 corner_radius, u32 corner_mask)
{
  dl->primitives.rounded_rects[dl->rounded_rects_count++] = {
      rect, get_current_scissor_idx(dl), color_to_int(color), corner_radius,
      corner_mask};

  return dl->rounded_rects_count - 1;
}

RoundedRectPrimitive *push_rounded_rect(DrawList *dl, i32 z, Rect rect,
                                        f32 corner_radius, Color color,
                                        u32 corner_mask = CornerMask::ALL)
{
  auto push_rect_vert = [](DrawList *dl, u32 primitive_index, u8 corner) {
    dl->verts[dl->vert_count++] = {(u32)PrimitiveIds::ROUNDED_RECT |
                                   CORNERS[corner] | primitive_index};
  };

  if (!overlaps(rect, get_current_scissor(dl))) {
    return nullptr;
  }

  u32 primitive_idx =
      push_primitive_rounded_rect(dl, rect, color, corner_radius, corner_mask);

  push_rect_vert(dl, primitive_idx, 0);
  push_rect_vert(dl, primitive_idx, 1);
  push_rect_vert(dl, primitive_idx, 2);
  push_rect_vert(dl, primitive_idx, 1);
  push_rect_vert(dl, primitive_idx, 3);
  push_rect_vert(dl, primitive_idx, 2);

  push_draw_call(dl, 2, z);

  return &dl->primitives.rounded_rects[primitive_idx];
}

RoundedRectPrimitive *push_rect(DrawList *dl, i32 z, Rect rect, Color color)
{
  return push_rounded_rect(dl, z, rect, 0, color);
}

u32 push_primitive_bitmap_glyph(DrawList *dl, Rect rect, Vec4f uv_bounds,
                                Color color)
{
  dl->primitives.bitmap_glyphs[dl->bitmap_glyphs_count++] = {
      rect, uv_bounds, get_current_scissor_idx(dl), color_to_int(color)};

  return dl->bitmap_glyphs_count - 1;
}

void push_bitmap_glyph(DrawList *dl, i32 z, Rect rect, Vec4f uv_bounds,
                       Color color)
{
  auto push_glyph_vert = [](DrawList *dl, u32 primitive_index, u8 corner) {
    dl->verts[dl->vert_count++] = {(u32)PrimitiveIds::BITMAP_GLYPH |
                                   CORNERS[corner] | primitive_index};
  };

  if (!overlaps(rect, get_current_scissor(dl))) {
    return;
  }

  u32 primitive_idx = push_primitive_bitmap_glyph(dl, rect, uv_bounds, color);

  push_glyph_vert(dl, primitive_idx, 0);
  push_glyph_vert(dl, primitive_idx, 1);
  push_glyph_vert(dl, primitive_idx, 2);
  push_glyph_vert(dl, primitive_idx, 1);
  push_glyph_vert(dl, primitive_idx, 3);
  push_glyph_vert(dl, primitive_idx, 2);

  push_draw_call(dl, 2, z);
}

void push_text(DrawList *dl, i32 z, String text, Vec2f pos, Color color,
               f32 height)
{
  f32 baseline     = dl->font.baseline;
  f32 resize_ratio = height / dl->font.font_size_px;
  for (int i = 0; i < text.size; i++) {
    Character c     = dl->font.characters[text.data[i]];
    Rect shape_rect = {
        pos.x + c.shape.x * resize_ratio, pos.y + (c.shape.y) * resize_ratio,
        c.shape.width * resize_ratio, c.shape.height * resize_ratio};
    pos.x += c.advance * resize_ratio;

    shape_rect.x = floorf(shape_rect.x);
    shape_rect.y = floorf(shape_rect.y);

    Vec4f uv_bounds = {c.uv.x, c.uv.y + c.uv.height, c.uv.x + c.uv.width,
                       c.uv.y};
    push_bitmap_glyph(dl, z, shape_rect, uv_bounds, color);
  }
};

void push_text_centered(DrawList *dl, i32 z, String text, Vec2f pos,
                        Vec2b center, Color color, f32 height)
{
  f32 resize_ratio = height / dl->font.font_size_px;
  f32 line_width   = get_text_width(dl->font, text, resize_ratio);

  Vec2f centered_pos;
  centered_pos.x = center.x ? pos.x - line_width / 2 : pos.x;
  centered_pos.y =
      center.y ? pos.y + (dl->font.ascent + dl->font.descent) / 2.f : pos.y;

  push_text(dl, z, text, centered_pos, color, height);
}

u32 push_primitive_vector_glyph(DrawList *dl, Rect rect, Glyph glyph,
                                Color color)
{
  dl->primitives.vector_glyphs[dl->vector_glyphs_count++] = {
      rect, glyph.curve_start_idx, glyph.curve_count, color_to_int(color),
      get_current_scissor_idx(dl)};

  return dl->vector_glyphs_count - 1;
}

void push_vector_glyph(DrawList *dl, i32 z, Rect rect, Glyph glyph, Color color)
{
  auto push_glyph_vert = [](DrawList *dl, u32 primitive_index, u8 corner) {
    dl->verts[dl->vert_count++] = {(u32)PrimitiveIds::VECTOR_GLYPH |
                                   CORNERS[corner] | primitive_index};
  };

  if (!overlaps(rect, get_current_scissor(dl))) {
    return;
  }

  u32 primitive_idx = push_primitive_vector_glyph(dl, rect, glyph, color);

  push_glyph_vert(dl, primitive_idx, 0);
  push_glyph_vert(dl, primitive_idx, 1);
  push_glyph_vert(dl, primitive_idx, 2);
  push_glyph_vert(dl, primitive_idx, 1);
  push_glyph_vert(dl, primitive_idx, 3);
  push_glyph_vert(dl, primitive_idx, 2);

  push_draw_call(dl, 2, z);
}

void push_vector_text(DrawList *dl, i32 z, String text, Vec2f pos, Color color,
                      f32 size)
{
  for (int i = 0; i < text.size; i++) {
    Glyph g = dl->vfont.glyphs[text.data[i]];

    f32 width       = size * g.size.x;
    f32 height      = size * g.size.y;
    Rect shape_rect = {pos.x + (size * g.bearing.x),
                       pos.y + (size * dl->vfont.ascent) - (size * g.bearing.y),
                       width, height};
    pos.x += size * g.advance;

    push_vector_glyph(dl, z, shape_rect, dl->vfont.glyphs[text.data[i]], color);
  }
};

void push_vector_text(DrawList *dl, VectorFont *font, i32 z, String text,
                      Vec2f pos, Color color, f32 size)
{
  for (int i = 0; i < text.size; i++) {
    Glyph g = font->glyphs[text.data[i]];
    g.curve_start_idx += font->char_buffer_offset;

    f32 width       = size * g.size.x;
    f32 height      = size * g.size.y;
    Rect shape_rect = {pos.x + (size * g.bearing.x),
                       pos.y + (size * font->ascent) - (size * g.bearing.y),
                       width, height};
    pos.x += size * g.advance;

    push_vector_glyph(dl, z, shape_rect, g, color);
  }
};

// axes: 0 for left, 1 for center, 2 for right
void push_vector_text_justified(DrawList *dl, i32 z, String text, Vec2f pos,
                                Color color, f32 size, Vec2i axes)
{
  f32 text_width = dl->vfont.get_text_width(text, size);
  if (axes.x == 2) {
    pos.x -= text_width;
  } else if (axes.x == 1) {
    pos.x -= text_width / 2.f;
  }

  if (axes.y == 2) {
    pos.y -= size;
  } else if (axes.y == 1) {
    pos.y -= size / 2.f;
  }

  for (int i = 0; i < text.size; i++) {
    Glyph g = dl->vfont.glyphs[text.data[i]];

    f32 width       = size * g.size.x;
    f32 height      = size * g.size.y;
    Rect shape_rect = {pos.x + (size * g.bearing.x),
                       pos.y + (size * dl->vfont.ascent) - (size * g.bearing.y),
                       width, height};
    pos.x += size * g.advance;

    push_vector_glyph(dl, z, shape_rect, dl->vfont.glyphs[text.data[i]], color);
  }
};

void push_vector_text_centered(DrawList *dl, i32 z, String text, Vec2f pos,
                               Color color, f32 size, Vec2b center)
{
  if (center.x) {
    f32 text_width = dl->vfont.get_text_width(text, size);
    pos.x -= text_width / 2;
  }
  if (center.y) pos.y -= size / 2;

  for (int i = 0; i < text.size; i++) {
    Glyph g = dl->vfont.glyphs[text.data[i]];

    f32 width       = size * g.size.x;
    f32 height      = size * g.size.y;
    Rect shape_rect = {pos.x + (size * g.bearing.x),
                       pos.y + (size * dl->vfont.ascent) - (size * g.bearing.y),
                       width, height};
    pos.x += size * g.advance;

    push_vector_glyph(dl, z, shape_rect, dl->vfont.glyphs[text.data[i]], color);
  }
};

void push_texture_rect(DrawList *dl, i32 z, Rect rect, Vec4f uv_bounds,
                       u32 texture_id)
{
  auto push_primitive_texture_rect = [](DrawList *dl, Rect rect,
                                        Vec4f uv_bounds, u32 texture_id) {
    dl->primitives.texture_rects[dl->texture_rects_count++] = {
        rect, uv_bounds, texture_id, get_current_scissor_idx(dl)};

    return dl->texture_rects_count - 1;
  };
  auto push_texture_rect_vert = [](DrawList *dl, u32 primitive_index,
                                   u8 corner) {
    dl->verts[dl->vert_count++] = {(u32)PrimitiveIds::TEXTURE_RECT |
                                   CORNERS[corner] | primitive_index};
  };

  if (!overlaps(rect, get_current_scissor(dl))) {
    return;
  }

  u32 primitive_idx =
      push_primitive_texture_rect(dl, rect, uv_bounds, texture_id);

  push_texture_rect_vert(dl, primitive_idx, 0);
  push_texture_rect_vert(dl, primitive_idx, 1);
  push_texture_rect_vert(dl, primitive_idx, 2);
  push_texture_rect_vert(dl, primitive_idx, 1);
  push_texture_rect_vert(dl, primitive_idx, 3);
  push_texture_rect_vert(dl, primitive_idx, 2);

  push_draw_call(dl, 2, z);
}

u32 push_primitive_line(DrawList *dl, Vec2f a, Vec2f b, Color color)
{
  dl->primitives.lines[dl->lines_count++] = {a, b, color_to_int(color),
                                             get_current_scissor_idx(dl)};

  return dl->lines_count - 1;
}

void push_line(DrawList *dl, i32 z, Vec2f a, Vec2f b, Color color)
{
  auto push_vert = [](DrawList *dl, u32 primitive_index, u8 corner) {
    dl->verts[dl->vert_count++] = {(u32)PrimitiveIds::LINE | CORNERS[corner] |
                                   primitive_index};
  };

  Vec2f mins        = min(a, b);
  Vec2f maxs        = max(a, b);
  Rect bounding_box = {mins.x, mins.y, maxs.x - mins.x, maxs.y - mins.y};
  if (!overlaps(bounding_box, get_current_scissor(dl))) {
    return;
  }

  u32 primitive_idx = push_primitive_line(dl, a, b, color);

  push_vert(dl, primitive_idx, 0);
  push_vert(dl, primitive_idx, 1);
  push_vert(dl, primitive_idx, 2);
  push_vert(dl, primitive_idx, 1);
  push_vert(dl, primitive_idx, 3);
  push_vert(dl, primitive_idx, 2);

  push_draw_call(dl, 2, z);
}

void push_cubic_spline(DrawList *dl, i32 z, Vec2f p[4], Color color,
                       i32 n_segments)
{
  auto evaluate_spline = [&](f32 t) {
    f32 t2 = t * t;
    f32 t3 = t2 * t;
    Vec2f a = (-p[0] + 3 * p[1] - 3 * p[2] + p[3]) * t3;
    Vec2f b = (3 * p[0] - 6 * p[1] + 3 * p[2]) * t2;
    Vec2f c = (-3 * p[0] + 3 * p[1]) * t;
    Vec2f d = p[0];

    return a + b + c + d;
  };

  for (i32 i = 0; i < n_segments; i++) {
    f32 t0  = (f32)i / n_segments;
    f32 t1  = (f32)(i + 1) / n_segments;
    Vec2f start = evaluate_spline(t0);
    Vec2f end = evaluate_spline(t1);

    push_line(dl, z, start, end, color);
  }
}

void init_draw_system(DrawList *dl, Gpu::Device *device, Gpu::Pipeline pipeline)
{
  // TODO: create pipeline here
  dl->desc_set = Gpu::create_descriptor_set(device, pipeline);

  dl->vfont = create_font("resources/fonts/OpenSans-Regular.ttf");
  for (i32 i = 0; i < dl->vfont.curves.size; i++) {
    dl->primitives.conic_curves[dl->conic_curves_count++] = {
        dl->vfont.curves[i].p0, dl->vfont.curves[i].p1, dl->vfont.curves[i].p2};
  }

  dl->icon_font = create_font("resources/fonts/fontello/fontello.ttf");
  dl->icon_font.char_buffer_offset = dl->conic_curves_count;
  for (i32 i = 0; i < dl->icon_font.curves.size; i++) {
    dl->primitives.conic_curves[dl->conic_curves_count++] = {
        dl->icon_font.curves[i].p0, dl->icon_font.curves[i].p1,
        dl->icon_font.curves[i].p2};
  }

  dl->primitive_buffer = create_storage_buffer(device, MB);
  bind_storage_buffer(device, dl->desc_set, dl->primitive_buffer, 1);

  dl->verts        = (u32 *)malloc(sizeof(u32) * 1024 * 1024);
  dl->index_buffer = create_index_buffer(device, MB);
}

void draw_system_start_frame(DrawList *dl)
{
  dl->vert_count = 0;
  dl->draw_calls.clear();
  dl->max_z = -1;

  dl->clip_rects_count    = 0;
  dl->rounded_rects_count = 0;
  dl->texture_rects_count = 0;
  dl->bitmap_glyphs_count = 0;
  dl->vector_glyphs_count = 0;
  dl->lines_count         = 0;

  dl->scissor_idxs.clear();
  dl->scissors.clear();

  push_scissor(dl, {0, 0, 100000, 100000});
}

void draw_system_end_frame(DrawList *dl, Gpu::Device *device,
                           Gpu::Pipeline pipeline, Vec2f canvas_size, u64 frame)
{
  if (dl->frame != frame) {
    dl->frame = frame;
    upload_buffer_staged(device, dl->primitive_buffer, &dl->primitives,
                         sizeof(dl->primitives));
  }

  Gpu::bind_descriptor_set(device, pipeline, dl->desc_set);
  Gpu::upload_buffer_staged(device, dl->index_buffer, dl->verts,
                            dl->vert_count * sizeof(u32));
  Gpu::push_constant(device, pipeline, &canvas_size, sizeof(Vec2f));

  for (i32 z = dl->max_z; z >= 0; z--) {
    for (i32 i = 0; i < dl->draw_calls.size; i++) {
      DrawCall call = dl->draw_calls[i];
      if (call.z == z) {
        Gpu::draw_indexed(device, dl->index_buffer, call.vert_offset,
                          call.tri_count * 3);
      }
    }
  }
}

}  // namespace Dui