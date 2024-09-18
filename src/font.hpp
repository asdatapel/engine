#pragma once

#define STB_TRUETYPE_IMPLEMENTATION
#include <third_party/stb/stb_truetype.h>

#include "file.hpp"
#include "image.hpp"
#include "platform.hpp"
#include "types.hpp"

const i32 NUM_CHARS_IN_FONT = 128;

struct Character {
  Engine::Rect shape;
  Engine::Rect uv;
  f32 advance;
};

struct Font {
  f32 font_size_px;

  f32 ascent;
  f32 descent;
  f32 baseline;

  Image atlas;
  Character characters[NUM_CHARS_IN_FONT];
};

Font load_font(String filepath, f32 size, Allocator *allocator)
{
  File file = read_file(filepath, allocator);

  Font font;
  font.font_size_px = size;

  stbtt_fontinfo stb_font;
  stbtt_InitFont(&stb_font, (unsigned char *)file.data.data, 0);
  f32 stb_scale = stbtt_ScaleForPixelHeight(&stb_font, size);

  i32 ascent, descent, lineGap;
  stbtt_GetFontVMetrics(&stb_font, &ascent, &descent, &lineGap);
  font.ascent  = ascent * stb_scale;
  font.descent = descent * stb_scale;

  i32 x0, y0, x1, y1;
  stbtt_GetFontBoundingBox(&stb_font, &x0, &y0, &x1, &y1);
  font.baseline = size + (y0 * stb_scale);

  font.atlas = Image(1024, 1024, sizeof(u8), allocator);
  stbtt_bakedchar cdata[96];
  i32 success = stbtt_BakeFontBitmap((unsigned char *)file.data.data, 0, size,
                                     font.atlas.data(), font.atlas.width,
                                     font.atlas.height, 32, 96,
                                     cdata);  // no guarantee this fits!
  assert(success);

  for (i32 i = 32; i < NUM_CHARS_IN_FONT; i++) {
    f32 x = 0;
    f32 y = 0;
    stbtt_aligned_quad q;
    stbtt_GetBakedQuad(cdata, font.atlas.width, font.atlas.height, i - 32, &x,
                       &y, &q, 0);

    font.characters[i].shape   = {q.x0, q.y0, q.x1 - q.x0, q.y1 - q.y0};
    font.characters[i].uv      = {q.s0, q.t1, q.s1 - q.s0, q.t0 - q.t1};
    font.characters[i].advance = x;
  }

  return font;
}

f32 get_text_width(const Font &font, String text, f32 scale = 1.f)
{
  f32 width = 0;
  for (i32 i = 0; i < text.size; i++) {
    width += font.characters[text.data[i]].advance;
  }
  return width * scale;
}

f32 max_ascent(const Font &font, String text)
{
  f32 ascent = 0;
  for (i32 i = 0; i < text.size; i++) {
    if (font.characters[text.data[i]].shape.y < ascent)
      ascent = font.characters[text.data[i]].shape.y;
  }
  return ascent;
}

f32 max_descent(const Font &font, String text)
{
  f32 descent = 0;
  for (i32 i = 0; i < text.size; i++) {
    if (font.characters[text.data[i]].shape.y +
            font.characters[text.data[i]].shape.height >
        descent)
      descent = font.characters[text.data[i]].shape.y +
                font.characters[text.data[i]].shape.height;
  }
  return descent;
}

f32 get_single_line_height(const Font &font, String text, f32 scale = 1.f)
{
  return (max_descent(font, text) - max_ascent(font, text)) * scale;
}

i32 char_index_at_pos(Font *font, String text, Vec2f text_pos, Vec2f pos,
                      f32 scale = 1.f)
{
  f32 cursor_x = text_pos.x;
  for (i32 i = 0; i < text.size; i++) {
    const Character &c_data = font->characters[text[i]];

    if (cursor_x + (c_data.advance * scale / 2.f) > pos.x) return i;

    cursor_x += c_data.advance * scale;
  }

  return text.size;
}

struct TextPlacer {
  Vec2f next_pos;

  Engine::Rect shape;
  Engine::Rect uv;
  TextPlacer(Vec2f pos) { this->next_pos = pos; }
};
TextPlacer place_character(Font *font, char c, f32 height, TextPlacer cursor)
{
  f32 resize_ratio = height / font->font_size_px;

  const Character &c_data = font->characters[c];
  cursor.shape            = {cursor.next_pos.x + c_data.shape.x * resize_ratio,
                             cursor.next_pos.y + (c_data.shape.y) * resize_ratio,
                             c_data.shape.width * resize_ratio,
                             c_data.shape.height * resize_ratio};
  cursor.next_pos.x += c_data.advance * resize_ratio;

  cursor.shape.x      = floorf(cursor.shape.x);
  cursor.shape.y      = floorf(cursor.shape.y);
  cursor.shape.width  = floorf(cursor.shape.width);
  cursor.shape.height = floorf(cursor.shape.height);

  cursor.uv = c_data.uv;

  return cursor;
}

// void draw_text_cropped(const Font &font, RenderTarget target, String text,
//                        f32 x, f32 y, f32 h_scale, f32 v_scale,
//                        b8 just_alpha = false)
// {
//   f32 ascent   = max_ascent(font, text);
//   f32 baseline = -max_ascent(font, text) * v_scale;
//   for (i32 i = 0; i < text.size; i++) {
//     Character c     = font.characters[text.data[i]];
//     Engine::Rect shape_rect = {x + c.shape.x, y + baseline + (v_scale * c.shape.y),
//                        h_scale * c.shape.width, v_scale * c.shape.height};
//     x += c.advance * h_scale;
//     if (just_alpha) {
//       draw_single_channel_text(target, shape_rect, c.uv, font.atlas);
//     } else {
//       draw_textured_mapped_rect(target, shape_rect, c.uv, font.atlas);
//     }
//   }
// }

// void draw_text(const Font &font, RenderTarget target, String text, f32 x, f32
// y,
//                f32 h_scale, f32 v_scale)
// {
//   f32 baseline = font.baseline * v_scale;
//   for (i32 i = 0; i < text.size; i++) {
//     Character c     = font.characters[text.data[i]];
//     Engine::Rect shape_rect = {x + c.shape.x, y + baseline + (v_scale * c.shape.y),
//                        h_scale * c.shape.width, v_scale * c.shape.height};
//     x += c.advance * h_scale;
//     draw_textured_mapped_rect(target, shape_rect, c.uv, font.atlas);
//   }
// }

// void draw_text(const Font &font, RenderTarget target, String text, f32 x, f32
// y,
//                Color color)
// {
//   f32 baseline = font.baseline;
//   for (i32 i = 0; i < text.size; i++) {
//     Character c     = font.characters[text.data[i]];
//     Engine::Rect shape_rect = {x + c.shape.x, y + baseline + c.shape.y,
//     c.shape.width,
//                        c.shape.height};
//     x += c.advance;
//     draw_textured_mapped_rect(target, shape_rect, c.uv, font.atlas, color);
//   }
// }

// void draw_centered_text(const Font &font, RenderTarget target, String text,
//                         Engine::Rect sub_target, f32 border, f32 scale,
//                         f32 aspect_ratio, b8 just_alpha = false)
// {
//   f32 border_x = border * sub_target.width;
//   f32 border_y = border * sub_target.height;

//   f32 width  = sub_target.width - (border_x * 2);
//   f32 height = sub_target.height - (border_y * 2);

//   f32 text_width  = get_text_width(font, text, scale);
//   f32 text_height = get_single_line_height(font, text, scale * aspect_ratio);

//   f32 oversize = width / text_width;
//   if (oversize < 1.f) {
//     scale *= oversize;
//     text_width *= oversize;
//     text_height *= oversize;
//   }

//   oversize = height / text_height;
//   if (oversize < 1.f) {
//     scale *= oversize;
//     text_width *= oversize;
//     text_height *= oversize;
//   }

//   f32 x_start = sub_target.x + border_x + ((width - text_width) / 2.f);
//   f32 y_start = sub_target.y + border_y + ((height - text_height) / 2.f);

//   draw_text_cropped(font, target, text, x_start, y_start, scale,
//                     scale * aspect_ratio, just_alpha);
// }