#pragma once

#include "external/stb/stb_truetype.hpp"

#include "platform.hpp"

const int NUM_CHARS_IN_FONT = 128;

struct Character {
  Rect shape;
  Rect uv;
  float advance;
};

struct Font {
  float font_size_px;

  float ascent;
  float descent;
  float baseline;

  Texture2D atlas;
  Character characters[NUM_CHARS_IN_FONT];
};

// Font load_font(FileData file, float size, StackAllocator *tmp_allocator)
// {
//   Font font;
//   font.font_size_px = size;

//   stbtt_fontinfo stb_font;
//   stbtt_InitFont(&stb_font, (unsigned char *)file.data, 0);
//   float stb_scale = stbtt_ScaleForPixelHeight(&stb_font, size);

//   int ascent, descent, lineGap;
//   stbtt_GetFontVMetrics(&stb_font, &ascent, &descent, &lineGap);
//   font.ascent  = ascent * stb_scale;
//   font.descent = descent * stb_scale;

//   int x0, y0, x1, y1;
//   stbtt_GetFontBoundingBox(&stb_font, &x0, &y0, &x1, &y1);
//   font.baseline = size + (y0 * stb_scale);

//   int bitmap_width = 1024, bitmap_height = 1024;
//   unsigned char *bitmap = (unsigned char *)tmp_allocator->alloc(bitmap_width * bitmap_height);
//   stbtt_bakedchar cdata[96];
//   int success = stbtt_BakeFontBitmap((unsigned char *)file.data, 0, size, bitmap, bitmap_width,
//                                      bitmap_height, 32, 96, cdata);  // no guarantee this fits!
//   assert(success);

//   font.atlas = Texture2D(bitmap_width, bitmap_height, TextureFormat::RED, true);
//   font.atlas.upload(bitmap, true);
//   for (int i = 32; i < NUM_CHARS_IN_FONT; i++) {
//     float x = 0;
//     float y = 0;
//     stbtt_aligned_quad q;
//     stbtt_GetBakedQuad(cdata, bitmap_width, bitmap_height, i - 32, &x, &y, &q, 1);

//     font.characters[i].shape   = {q.x0, q.y0, q.x1 - q.x0, q.y1 - q.y0};
//     font.characters[i].uv      = {q.s0, q.t1, q.s1 - q.s0, q.t0 - q.t1};
//     font.characters[i].advance = x;
//   }

//   return font;
// }

// float get_text_width(const Font &font, String text, float scale = 1.f)
// {
//   float width = 0;
//   for (int i = 0; i < text.len; i++) {
//     width += font.characters[text.data[i]].advance;
//   }
//   return width * scale;
// }

// float max_ascent(const Font &font, String text)
// {
//   float ascent = 0;
//   for (int i = 0; i < text.len; i++) {
//     if (font.characters[text.data[i]].shape.y < ascent)
//       ascent = font.characters[text.data[i]].shape.y;
//   }
//   return ascent;
// }

// float max_descent(const Font &font, String text)
// {
//   float descent = 0;
//   for (int i = 0; i < text.len; i++) {
//     if (font.characters[text.data[i]].shape.y + font.characters[text.data[i]].shape.height >
//         descent)
//       descent = font.characters[text.data[i]].shape.y + font.characters[text.data[i]].shape.height;
//   }
//   return descent;
// }

// float get_single_line_height(const Font &font, String text, float scale = 1.f)
// {
//   return (max_descent(font, text) - max_ascent(font, text)) * scale;
// }

// void draw_text_cropped(const Font &font, RenderTarget target, String text, float x, float y,
//                        float h_scale, float v_scale, b8 just_alpha = false)
// {
//   float ascent   = max_ascent(font, text);
//   float baseline = -max_ascent(font, text) * v_scale;
//   for (int i = 0; i < text.len; i++) {
//     Character c     = font.characters[text.data[i]];
//     Rect shape_rect = {x + c.shape.x, y + baseline + (v_scale * c.shape.y), h_scale * c.shape.width,
//                        v_scale * c.shape.height};
//     x += c.advance * h_scale;
//     if (just_alpha) {
//       draw_single_channel_text(target, shape_rect, c.uv, font.atlas);
//     } else {
//       draw_textured_mapped_rect(target, shape_rect, c.uv, font.atlas);
//     }
//   }
// }

// void draw_text(const Font &font, RenderTarget target, String text, float x, float y, float h_scale,
//                float v_scale)
// {
//   float baseline = font.baseline * v_scale;
//   for (int i = 0; i < text.len; i++) {
//     Character c     = font.characters[text.data[i]];
//     Rect shape_rect = {x + c.shape.x, y + baseline + (v_scale * c.shape.y), h_scale * c.shape.width,
//                        v_scale * c.shape.height};
//     x += c.advance * h_scale;
//     draw_textured_mapped_rect(target, shape_rect, c.uv, font.atlas);
//   }
// }

// void draw_text(const Font &font, RenderTarget target, String text, float x, float y, Color color)
// {
//   float baseline = font.baseline;
//   for (int i = 0; i < text.len; i++) {
//     Character c     = font.characters[text.data[i]];
//     Rect shape_rect = {x + c.shape.x, y + baseline + c.shape.y, c.shape.width, c.shape.height};
//     x += c.advance;
//     draw_textured_mapped_rect(target, shape_rect, c.uv, font.atlas, color);
//   }
// }

// void draw_centered_text(const Font &font, RenderTarget target, String text, Rect sub_target,
//                         float border, float scale, float aspect_ratio, b8 just_alpha = false)
// {
//   float border_x = border * sub_target.width;
//   float border_y = border * sub_target.height;

//   float width  = sub_target.width - (border_x * 2);
//   float height = sub_target.height - (border_y * 2);

//   float text_width  = get_text_width(font, text, scale);
//   float text_height = get_single_line_height(font, text, scale * aspect_ratio);

//   float oversize = width / text_width;
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

//   float x_start = sub_target.x + border_x + ((width - text_width) / 2.f);
//   float y_start = sub_target.y + border_y + ((height - text_height) / 2.f);

//   draw_text_cropped(font, target, text, x_start, y_start, scale, scale * aspect_ratio, just_alpha);
// }