#pragma once

#include <algorithm>
#include <execution>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "input.hpp"
#include "logging.hpp"

FT_Library library;

struct QuadCurve2 {
  Vec2f p0, p1, p2;
};

struct Glyph {
  u32 curve_start_idx;
  u32 curve_count;

  Vec2f size;
  f32 advance;
  Vec2f bearing;
};

struct VectorFont {
  Array<QuadCurve2, 4096> curves;
  Array<Glyph, 256> glyphs;

  f32 ascent;

  f32 get_text_width(String text, f32 scale = 1.f)
  {
    f32 width = 0;
    for (i32 i = 0; i < text.size; i++) {
      Glyph g = glyphs[text[i]];
      width += g.advance;
    }

    return width * scale;
  }

  i32 char_index_at_pos(String text, Vec2f text_pos, Vec2f pos, f32 scale = 1.f)
  {
    f32 cursor_x = text_pos.x;
    for (i32 i = 0; i < text.size; i++) {
      Glyph g = glyphs[text[i]];

      if (cursor_x + (g.advance * scale / 2.f) > pos.x) return i;

      cursor_x += g.advance * scale;
    }

    return text.size;
  }
};

Glyph extract_glyph(VectorFont* font, FT_Face face, u32 character)
{
  FT_Error err = FT_Set_Pixel_Sizes(face, 0, 32);
  if (err) {
    fatal("failed to set pixel size?");
  }

  u32 glyph_index = FT_Get_Char_Index(face, character);

  err = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
  if (err) {
    fatal("failed to load glyph");
  }

  FT_Outline outline = face->glyph->outline;

  Glyph glyph;
  glyph.size    = Vec2f((f32)face->glyph->metrics.width / face->size->metrics.height,
                        (f32)face->glyph->metrics.height / face->size->metrics.height);
  glyph.bearing = Vec2f((f32)face->glyph->metrics.horiBearingX / face->size->metrics.height,
                        (f32)face->glyph->metrics.horiBearingY / face->size->metrics.height);
  glyph.advance = (f32)face->glyph->advance.x / face->size->metrics.height;

  glyph.curve_start_idx = font->curves.size;
  glyph.curve_count     = 0;

  for (i32 contour_i = 0; contour_i < outline.n_contours; contour_i++) {
    i32 contour_start =
        contour_i == 0 ? 0 : outline.contours[contour_i - 1] + 1;
    i32 contour_end = outline.contours[contour_i];

    b8 on_ghost_point = false;
    Vec2f ghost_point;

    i32 point_i = contour_start;
    while (point_i <= contour_end) {
      auto get_pos = [&](FT_Vector ft_vec) {
        f32 x = ((f32)ft_vec.x - face->glyph->metrics.horiBearingX) /
                face->glyph->metrics.width * .98 + .01;
        f32 y = ((f32)ft_vec.y + (face->glyph->metrics.height -
                                  face->glyph->metrics.horiBearingY)) /
                face->glyph->metrics.height * .98 + .01;

        return Vec2f(x, y);
      };

      Vec2f a = get_pos(outline.points[point_i]);
      Vec2f b;
      char b_tag;
      if (point_i + 1 > contour_end) {
        b     = get_pos(outline.points[contour_start]);
        b_tag = outline.tags[contour_start];
      } else {
        b     = get_pos(outline.points[point_i + 1]);
        b_tag = outline.tags[point_i + 1];
      }

      if (FT_CURVE_TAG(b_tag) == FT_CURVE_TAG_ON) {
        font->curves.push_back({a, (a + b) / Vec2f(2.f, 2.f), b});
        glyph.curve_count++;
        point_i++;
      } else if (FT_CURVE_TAG(b_tag) == FT_CURVE_TAG_CONIC) {
        if (on_ghost_point) {
          a              = ghost_point;
          on_ghost_point = false;
        }

        Vec2f c;
        char c_tag;
        if (point_i + 2 > contour_end) {
          c     = get_pos(outline.points[contour_start]);
          c_tag = outline.tags[contour_start];
        } else {
          c     = get_pos(outline.points[point_i + 2]);
          c_tag = outline.tags[point_i + 2];
        }

        if (FT_CURVE_TAG(c_tag) == FT_CURVE_TAG_CONIC) {
          on_ghost_point = true;
          ghost_point    = (b + c) / Vec2f(2.f, 2.f);
          c              = ghost_point;
        } else if (FT_CURVE_TAG(c_tag) == FT_CURVE_TAG_ON) {
          point_i++;
        } else {
          fatal("dumb");
        }

        font->curves.push_back({a, b, c});
        glyph.curve_count++;

        point_i++;
      }
    }
  }

  return glyph;
}

VectorFont create_font()
{
  FT_Error err = FT_Init_FreeType(&library);
  if (err) {
    fatal("failed to init freetype");
  }

  FT_Face face;
  err = FT_New_Face(library, "resources/fonts/roboto/Roboto-Regular.ttf", 0,
                    &face);
  if (err) {
    fatal("failed to load font");
  }

  VectorFont font;
  font.ascent = (f32)face->ascender / face->height;
  for (i32 i = 0; i < 128; i++) {
    font.glyphs.push_back(extract_glyph(&font, face, i));
  }

  return font;
}

// struct ConicCurve {
//   Vec2f p0, p1, control;
// };

// template <u32 WIDTH, u32 HEIGHT>
// struct Grid {
//   struct Cell {
//     Array<ConicCurve, 128> curves;
//     b8 inside = false;
//   };

//   const u32 width  = WIDTH;
//   const u32 height = HEIGHT;

//   Cell cells[WIDTH * HEIGHT];
//   Cell& get(u32 x, u32 y) { return cells[y * HEIGHT + x]; }
// };

// const Vec2i grid_dimensions = Vec2i(5, 5);
// const Vec2f cell_size =
//     Vec2f(1, 1) / Vec2f(grid_dimensions.x, grid_dimensions.y);
// Grid<5, 5> grid;

// // basically copied from https://github.com/GreenLightning/gpu-font-rendering
// float computeCoverage(ConicCurve curve, Vec2f origin, f32 rotation,
//                       f32 pixel_width)
// {
//   f32 cos_theta = cosf(rotation);
//   f32 sin_theta = sinf(rotation);

//   auto rotate = [&](Vec2f p) {
//     Vec2f p2;
//     p2.x = p.x * cos_theta - p.y * sin_theta;
//     p2.y = p.x * sin_theta + p.y * cos_theta;
//     return p2;
//   };

//   curve.p0 -= origin;
//   curve.p1 -= origin;
//   curve.control -= origin;

//   curve.p0      = rotate(curve.p0);
//   curve.p1      = rotate(curve.p1);
//   curve.control = rotate(curve.control);

//   Vec2f p0 = curve.p0;
//   Vec2f p1 = curve.control;
//   Vec2f p2 = curve.p1;

//   if (p0.y > 0 && p1.y > 0 && p2.y > 0) return 0.0;
//   if (p0.y < 0 && p1.y < 0 && p2.y < 0) return 0.0;

//   // Note: Simplified from abc formula by extracting a factor of (-2) from b.
//   Vec2f a = p0 - 2 * p1 + p2;
//   Vec2f b = p0 - p1;
//   Vec2f c = p0;

//   float t0, t1;
//   if (abs(a.y) >= 1e-5) {
//     // Quadratic segment, solve abc formula to find roots.
//     float radicand = b.y * b.y - a.y * c.y;
//     if (radicand <= 0) return 0.0;

//     float s = sqrt(radicand);
//     t0      = (b.y - s) / a.y;
//     t1      = (b.y + s) / a.y;
//   } else {
//     float t = p0.y / (p0.y - p2.y);
//     if (p0.y < p2.y) {
//       t0 = -1.0;
//       t1 = t;
//     } else {
//       t0 = t;
//       t1 = -1.0;
//     }
//   }

//   float alpha = 0;

//   if (t0 >= 0 && t0 < 1) {
//     float x = (a.x * t0 - 2.0 * b.x) * t0 + c.x;
//     alpha += clamp(x * (1.f / pixel_width) + .5f, 0.f, 1.f);
//   }

//   if (t1 >= 0 && t1 < 1) {
//     float x = (a.x * t1 - 2.0 * b.x) * t1 + c.x;
//     alpha -= clamp(x * (1.f / pixel_width) + .5f, 0.f, 1.f);
//   }

//   return alpha;
// }

// f32 rasterize(Image image, Input* input)
// {
//   static b8 init = false;
//   if (!init) {
//     init         = true;
//     FT_Error err = FT_Init_FreeType(&library);
//     if (err) {
//       fatal("failed to init freetype");
//     }
//     err = FT_New_Face(library, "resources/fonts/roboto/Roboto-Regular.ttf",
//     0,
//                       &face);
//     if (err) {
//       fatal("failed to load font");
//     }
//   }

//   static char c = 'q';
//   if (input->text_input.size > 0) {
//     c = input->text_input[0];
//   }

//   FT_Error err    = FT_Set_Pixel_Sizes(face, /* handle to face object */
//                                        0,    /* pixel_width           */
//                                        32);  /* pixel_height          */
//   u32 glyph_index = FT_Get_Char_Index(face, c);
//   err             = FT_Load_Glyph(face,             /* handle to face object
//   */
//                                   glyph_index,      /* glyph index */
//                                   FT_LOAD_DEFAULT); /* load flags, see below
//                                   */

//   if (face->glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
//     fatal("no glyph outline");
//   }
//   FT_Outline outline = face->glyph->outline;

//   Array<ConicCurve, 256> curves;

//   for (i32 contour_i = 0; contour_i < outline.n_contours; contour_i++) {
//     i32 contour_start =
//         contour_i == 0 ? 0 : outline.contours[contour_i - 1] + 1;
//     i32 contour_end = outline.contours[contour_i];

//     b8 on_ghost_point = false;
//     Vec2f ghost_point;

//     i32 point_i = contour_start;
//     while (point_i <= contour_end) {
//       if (FT_CURVE_TAG(outline.tags[point_i]) == FT_CURVE_TAG_ON)
//         printf("ON, ");
//       if (FT_CURVE_TAG(outline.tags[point_i]) == FT_CURVE_TAG_CONIC)
//         printf("CONIC, ");
//       if (FT_CURVE_TAG(outline.tags[point_i]) == FT_CURVE_TAG_CUBIC)
//         printf("CUBIC, ");

//       point_i++;
//     }
//   }

//   for (i32 contour_i = 0; contour_i < outline.n_contours; contour_i++) {
//     i32 contour_start =
//         contour_i == 0 ? 0 : outline.contours[contour_i - 1] + 1;
//     i32 contour_end = outline.contours[contour_i];

//     b8 on_ghost_point = false;
//     Vec2f ghost_point;

//     i32 point_i = contour_start;
//     while (point_i <= contour_end) {
//       auto get_pos = [&](FT_Vector ft_vec) {
//         f32 x = ((f32)ft_vec.x - face->glyph->metrics.horiBearingX) /
//                 face->glyph->metrics.width;
//         f32 y = ((f32)ft_vec.y + (face->glyph->metrics.height -
//                                   face->glyph->metrics.horiBearingY)) /
//                 face->glyph->metrics.height;

//         return Vec2f(x, y);
//       };

//       Vec2f a = get_pos(outline.points[point_i]);
//       Vec2f b;
//       char b_tag;
//       if (point_i + 1 > contour_end) {
//         b     = get_pos(outline.points[contour_start]);
//         b_tag = outline.tags[contour_start];
//       } else {
//         b     = get_pos(outline.points[point_i + 1]);
//         b_tag = outline.tags[point_i + 1];
//       }

//       if (FT_CURVE_TAG(b_tag) == FT_CURVE_TAG_ON) {
//         curves.push_back({a, b, (a + b) / Vec2f(2.f, 2.f)});
//         point_i++;
//       } else if (FT_CURVE_TAG(b_tag) == FT_CURVE_TAG_CONIC) {
//         if (on_ghost_point) {
//           a              = ghost_point;
//           on_ghost_point = false;
//         }

//         Vec2f c;
//         char c_tag;
//         if (point_i + 2 > contour_end) {
//           c     = get_pos(outline.points[contour_start]);
//           c_tag = outline.tags[contour_start];
//         } else {
//           c     = get_pos(outline.points[point_i + 2]);
//           c_tag = outline.tags[point_i + 2];
//         }

//         if (FT_CURVE_TAG(c_tag) == FT_CURVE_TAG_CONIC) {
//           on_ghost_point = true;
//           ghost_point    = (b + c) / Vec2f(2.f, 2.f);
//           c              = ghost_point;
//         } else if (FT_CURVE_TAG(c_tag) == FT_CURVE_TAG_ON) {
//           point_i++;
//         } else {
//           fatal("dumb");
//         }

//         curves.push_back({a, c, b});

//         point_i++;
//       }
//     }
//   }

//   struct Roots {
//     i8 num_roots = 0;
//     f32 t0;
//     f32 t1;

//     f32 x0;
//     f32 x1;

//     b8 completely_below;
//   };
//   auto get_roots = [](ConicCurve curve, Vec2f origin, f32 rotation) {
//     Roots roots;

//     f32 cos_theta = cosf(rotation);
//     f32 sin_theta = sinf(rotation);

//     auto rotate = [&](Vec2f p) {
//       Vec2f p2;
//       p2.x = p.x * cos_theta - p.y * sin_theta;
//       p2.y = p.x * sin_theta + p.y * cos_theta;
//       return p2;
//     };

//     curve.p0 -= origin;
//     curve.p1 -= origin;
//     curve.control -= origin;

//     curve.p0      = rotate(curve.p0);
//     curve.p1      = rotate(curve.p1);
//     curve.control = rotate(curve.control);

//     roots.completely_below =
//         (curve.p0.y <= 0 && curve.p1.y <= 0 && curve.control.y <= 0);

//     Vec2f a = curve.p0 - (2 * curve.control) + curve.p1;
//     Vec2f b = (2 * curve.control) - (2 * curve.p0);
//     Vec2f c = curve.p0;

//     // straight line
//     if (fabs(a.y) < 0.0000005f) {
//       roots.t0 = (-c.y) / b.y;
//       roots.x0 = (b.x * roots.t0) + c.x;
//       roots.num_roots++;
//       return roots;
//     };

//     f32 det = (b.y * b.y) - (4 * a.y * c.y);
//     // if (det < 0.f) return roots;

//     roots.t0 = (-b.y + sqrt(det)) / (2 * a.y);
//     roots.x0 = (a.x * roots.t0 * roots.t0) + (b.x * roots.t0) + c.x;
//     roots.num_roots++;

//     if (fabsf(det) < 0.00001f) return roots;

//     roots.t1 = (-b.y - sqrt(det)) / (2 * a.y);
//     roots.x1 = (a.x * roots.t1 * roots.t1) + (b.x * roots.t1) + c.x;
//     roots.num_roots++;

//     return roots;
//   };

//   // determine whether each grid cell is inside
//   // for (i32 y = 0; y < grid.height; y++) {
//   //   for (i32 x = 0; x < grid.width; x++) {
//   //     Vec2f cell_size   = Vec2f(1, 1) / Vec2f(grid.width, grid.height);
//   //     Vec2f cell_center = cell_size * Vec2f(x + .5f, y + .5f);

//   //     int DEBUG_Y = 51;
//   //     if (y == DEBUG_Y) {
//   //       y = y;
//   //     }

//   //     b8 inside = false;
//   //     i8 wind   = 0;
//   //     for (i32 i = 0; i < curves.size; i++) {
//   //       if (y == DEBUG_Y && x == 0) {
//   //         printf("%i: ", i);

//   //         if (i == 22) {
//   //           printf("asdas");
//   //         }
//   //       }
//   //       Roots roots = get_roots(curves[i], cell_center, 0);

//   //       if (roots.num_roots > 0 && roots.t0 >= 0 && roots.t0 < 1 &&
//   //           roots.x0 > 0) {
//   //         inside = !inside;

//   //         if (y == DEBUG_Y && x == 0) {
//   //           printf("t0 %f", roots.t0);
//   //         }
//   //       }
//   //       if (roots.num_roots > 1 && roots.t1 >= 0 && roots.t1 < 1 &&
//   //           roots.x1 > 0) {
//   //         inside = !inside;
//   //         if (y == DEBUG_Y && x == 0) {
//   //           printf(" t1 %f", roots.t1);
//   //         }
//   //       }

//   //       if (y == DEBUG_Y && x == 0) {
//   //         printf(", ");
//   //       }
//   //     }

//   //     // if (y == DEBUG_Y) inside = true;

//   //     grid.get(x, y).inside = inside;
//   //   }
//   // }
//   // printf("\n");

//   // scanline for grid cell centers in or out
//   for (i32 y = 0; y < grid.height; y++) {
//     f32 cell_center_y = (y + .5f) * (1.f / grid.height);

//     Array<f32, 32> sorted_curves;

//     for (i32 i = 0; i < curves.size; i++) {
//       Roots roots = get_roots(curves[i], Vec2f(0, cell_center_y), 0);

//       if (roots.num_roots > 0 && !roots.completely_below && roots.t0 >= 0 &&
//           roots.t0 <= 1 && roots.x0 > .0f) {
//         f32 x = roots.x0;

//         i32 index = 0;
//         while (index < sorted_curves.size && sorted_curves[index] < x) {
//           index++;
//         }
//         sorted_curves.insert(index, x);
//       }

//       if (roots.num_roots > 1 && !roots.completely_below && roots.t1 >= 0 &&
//           roots.t1 <= 1 && roots.x1 > .0f) {
//         f32 x = roots.x1;

//         i32 index = 0;
//         while (index < sorted_curves.size && sorted_curves[index] < x) {
//           index++;
//         }

//         sorted_curves.insert(index, x);
//       }
//     }

//     sorted_curves.push_back(1);

//     f32 x = 0.f;
//     b8 in = false;
//     // for (i32 i = 0; i < sorted_curves.size; i++) {
//     //   i32 grid_x_min = ceilf(x * grid.width - .5f);
//     //   i32 grid_x_max = ceilf(sorted_curves[i] * grid.width - .5f);

//     //   for (i32 i = grid_x_min; i < grid_x_max; i++) {
//     //     grid.get(i, y).inside = in;
//     //   }

//     //   in = !in;
//     //   x  = sorted_curves[i];
//     // }

//     // add to grid struct
//     x  = 0.f;
//     in = false;
//     while (x < 1.f) {
//       f32 min_hit_x = INFINITY;

//       for (i32 i = 0; i < curves.size; i++) {
//         Roots roots = get_roots(curves[i], Vec2f(x, cell_center_y), 0);

//         if (roots.num_roots > 0 && roots.t0 >= 0 && roots.t0 <= 1 &&
//             roots.x0 > .0000001f) {
//           min_hit_x = fminf(min_hit_x, x + roots.x0);
//         }
//         if (roots.num_roots > 1 && roots.t1 >= 0 && roots.t1 <= 1 &&
//             roots.x1 > .0000001f) {
//           min_hit_x = fminf(min_hit_x, x + roots.x1);
//         }
//       }

//       i32 grid_x_min = ceilf(x * grid.width - .5f);
//       i32 grid_x_max = grid.width - 1;
//       if (min_hit_x != INFINITY) {
//         grid_x_max = fminf(grid_x_max, ceilf(min_hit_x * grid.width - .5f));
//       }

//       for (i32 i = grid_x_min; i <= grid_x_max; i++) {
//         grid.get(i, y).inside = in;
//       }

//       in = !in;
//       x  = min_hit_x;
//     }
//   }

//   auto does_intersect = [&](ConicCurve curve, Vec2f origin, f32 rotation,
//                             Vec2f x_range = {-1.f, 1.f}) {
//     Roots roots = get_roots(curve, origin, rotation);

//     if (roots.num_roots > 0 && !roots.completely_below) {
//       if (roots.t0 >= 0 && roots.t0 <= 1 && roots.x0 >= x_range.x &&
//           roots.x0 <= x_range.y) {
//         return true;
//       }
//     }

//     if (roots.num_roots > 1 && !roots.completely_below) {
//       if (roots.t1 >= 0 && roots.t1 <= 1 && roots.x1 >= x_range.x &&
//           roots.x1 <= x_range.y) {
//         return true;
//       }
//     }

//     return false;
//   };

//   for (i32 grid_y = 0; grid_y < grid_dimensions.y; grid_y++) {
//     for (i32 grid_x = 0; grid_x < grid_dimensions.x; grid_x++) {
//       grid.get(grid_x, grid_y).curves.clear();

//       Vec2f top_left     = Vec2f(grid_x, grid_y) * cell_size;
//       Vec2f top_right    = Vec2f(grid_x + 1, grid_y) * cell_size;
//       Vec2f bottom_left  = Vec2f(grid_x, grid_y + 1) * cell_size;
//       Vec2f bottom_right = Vec2f(grid_x + 1, grid_y + 1) * cell_size;

//       for (i32 curve_i = 0; curve_i < curves.size; curve_i++) {
//         ConicCurve curve = curves[curve_i];

//         b8 top_intersects =
//             does_intersect(curve, top_left, 0, {0, cell_size.x});
//         b8 bottom_intersects =
//             does_intersect(curve, bottom_left, 0, {0, cell_size.x});
//         b8 left_intersects =
//             does_intersect(curve, top_left, -3.1415f / 2.f, {0,
//             cell_size.x});
//         b8 right_intersects =
//             does_intersect(curve, top_right, -3.1415f / 2.f, {0,
//             cell_size.x});
//         b8 starts_in_rect = in_rect(
//             curve.p0, {top_left.x, top_left.y, cell_size.x, cell_size.y});

//         if (starts_in_rect || top_intersects || bottom_intersects ||
//             left_intersects || right_intersects) {
//           grid.get(grid_x, grid_y).curves.push_back(curve);
//         }
//       }
//     }
//   }

//   i32 grid_cell_size_x = image.width / grid.width;
//   i32 grid_cell_size_y = image.height / grid.height;

//   for (i32 y = 0; y < image.height; y++) {
//     for (i32 x = 0; x < image.width; x++) {
//       i32 grid_cell_x = (x + .5f) / image.width * grid_dimensions.x;
//       i32 grid_cell_y = (y + .5f) / image.height * grid_dimensions.y;

//       Array<ConicCurve, 128>& cell_curves =
//           grid.get(grid_cell_x, grid_cell_y).curves;

//       Vec2f pixel_center =
//           Vec2f((x + .5f) / image.width, (y + .5f) / image.height);

//       Vec2f cell_center =
//           Vec2f((grid_cell_x + .5f) * grid_cell_size_x / image.width,
//                 (grid_cell_y + .5f) * grid_cell_size_y / image.height);

//       u32 hits = 0;

//       Vec2f grid_diff = pixel_center - cell_center;
//       f32 angle       = -atan2(grid_diff.y, grid_diff.x);
//       f32 distance =
//           sqrtf(grid_diff.x * grid_diff.x + grid_diff.y * grid_diff.y);
//       b8 inside = grid.get(grid_cell_x, grid_cell_y).inside;
//       for (i32 curve_i = 0; curve_i < cell_curves.size; curve_i++) {
//         Roots roots = get_roots(cell_curves[curve_i], cell_center, angle);

//         if (roots.num_roots > 0 && roots.t0 >= 0 && roots.t0 <= 1 &&
//             roots.x0 > 0.f && roots.x0 <= distance)
//           inside = !inside;
//         if (roots.num_roots > 1 && roots.t1 >= 0 && roots.t1 <= 1 &&
//             roots.x1 > 0.f && roots.x1 <= distance)
//           inside = !inside;
//       }

//       // i32 n_samples = 8;
//       // f32 coverage  = 0.f;

//       // for (i32 curve_i = 0; curve_i < cell_curves.size; curve_i++) {
//       //   f32 this_curve_coverage = 0;

//       //   for (i32 s = 0; s < n_samples; s++) {
//       //     f32 angle   = s * 2 * 3.1415 / n_samples;
//       //     Roots roots = get_roots(cell_curves[curve_i], pixel_center,
//       angle);

//       //     f32 half_px = .5f / image.width;
//       //     f32 full_px = 1.f / image.width;

//       //     if (roots.num_roots > 0 && roots.t0 >= 0 && roots.t0 <= 1 &&
//       //         roots.x0 >= -half_px && roots.x0 <= half_px) {
//       //       f32 remaining_distance = roots.x0 - (-half_px);
//       //       this_curve_coverage +=
//       //           clamp(remaining_distance / full_px, 0.f, 1.f) / n_samples;
//       //     }
//       //     if (roots.num_roots > 1 && roots.t1 >= 0 && roots.t1 <= 1 &&
//       //         roots.x1 >= -half_px && roots.x1 <= half_px) {
//       //       f32 remaining_distance = roots.x1 - (-half_px);

//       //       this_curve_coverage -=
//       //           clamp(remaining_distance / full_px, 0.f, 1.f) / n_samples;
//       //     }
//       //   }

//       //   coverage = std::max(coverage, this_curve_coverage);
//       // }
//       // if (inside) coverage = 1 - coverage;
//       // hits = coverage * 8;

//       i32 n_samples = 2;
//       f32 coverage  = 0.f;
//       for (i32 curve_i = 0; curve_i < curves.size; curve_i++) {
//         f32 this_curve_coverage = 0;

//         for (i32 s = 0; s < n_samples; s++) {
//           f32 angle = s * 2 * 3.1415 / n_samples;

//           f32 half_px = .5f / image.width;

//           coverage += computeCoverage(curves[curve_i], pixel_center, angle,
//                                       1.f / image.width) /
//                       n_samples;
//         }
//       }
//       hits = clamp(coverage, .0f, 1.f) * 255;

//       // i32 samples_per_dimension = 8;
//       // for (i32 sample_y = 0; sample_y < samples_per_dimension; sample_y++)
//       {
//       //   for (i32 sample_x = 0; sample_x < samples_per_dimension;
//       sample_x++)
//       //   {
//       //     Vec2f offset =
//       //         Vec2f((sample_x + 1.f) * (1 / (samples_per_dimension
//       + 2.f)),
//       //               (sample_y + 1.f) * (1 / (samples_per_dimension
//       + 2.f)));
//       //     Vec2f sample_pos =
//       //         Vec2f((x + offset.x) / image.width, (y + offset.y) /
//       //         image.height);
//       //     Vec2f cell_center =
//       //         Vec2f((grid_cell_x + .5f) * grid_cell_size_x / image.width,
//       //               (grid_cell_y + .5f) * grid_cell_size_y /
//       image.height);

//       //     Vec2f grid_diff = sample_pos - cell_center;
//       //     f32 angle       = -atan2(grid_diff.y, grid_diff.x);
//       //     f32 distance =
//       //         sqrtf(grid_diff.x * grid_diff.x + grid_diff.y *
//       grid_diff.y);
//       //     b8 inside = grid.get(grid_cell_x, grid_cell_y).inside;
//       //     for (i32 curve_i = 0; curve_i < cell_curves.size; curve_i++) {
//       //       Roots roots = get_roots(cell_curves[curve_i], cell_center,
//       //       angle);

//       //       if (roots.num_roots > 0 && roots.t0 >= 0 && roots.t0 <= 1 &&
//       //           roots.x0 > 0.f && roots.x0 <= distance)
//       //         inside = !inside;
//       //       if (roots.num_roots > 1 && roots.t1 >= 0 && roots.t1 <= 1 &&
//       //           roots.x1 > 0.f && roots.x1 <= distance)
//       //         inside = !inside;
//       //     }

//       //     hits += inside;
//       //   }
//       // }

//       // for (i32 curve_i = 0; curve_i < cell_curves.size; curve_i++) {
//       //   u32 this_curve_hits = 0;
//       //   for (i32 s = 0; s < 8; s++) {
//       //     if (does_intersect(cell_curves[curve_i], pixel_center,
//       //                        -s * 2 * (3.1415 / 8),
//       //                        Vec2f(-.5f / image.width, .5f /
//       image.width))) {
//       //       this_curve_hits++;
//       //     }
//       //   }

//       //   hits = std::max(hits, this_curve_hits);
//       // }
//       // if (inside) hits = 8 - hits;

//       // u32 curve_hits = 0;
//       // for (i32 curve_i = 0; curve_i < curves.size; curve_i++) {
//       //   f32 outX[2];
//       //   i32 this_curve_hits =
//       //       IntersectHorz(curves[curve_i], (y + .5f) / image.height,
//       outX);

//       //   if (this_curve_hits > 0 && outX[0] > (x + .5f) / image.width)
//       //     curve_hits++;
//       //   if (this_curve_hits > 1 && outX[1] > (x + .5f) / image.width)
//       //     curve_hits++;
//       // }
//       // hits = (curve_hits % 2) * 8;

//       // u32 cell_inside = inside ? 150 : 0;
//       // u32 on_grid     = (x % grid_cell_size_x < 3) || (y %
//       grid_cell_size_y <
//       // 3)
//       //                       ? 0x00FF0000
//       //                       : 0;
//       // hits = inside ? 255 : 0;
//       ((u32*)image.data())[(image.height - y - 1) * image.width + x] =
//           ((hits) << 24) | 0x00FFFFFF;
//     }
//   }

//   // for (i32 i = 0; i < curves.size; i++) {
//   //   ConicCurve curve = curves[i];

//   //   auto convert = [&](Vec2f p) {
//   //     return Vec2i(clamp(p.x * image.width, 0.f, image.width - 1.f),
//   //                  clamp(p.y * image.height, 0.f, image.height - 1.f));
//   //   };
//   //   Vec2i p = convert(curve.p0);
//   //   ((u32*)image.data())[(image.height - p.y - 1) * image.width + p.x] =
//   //       0xFF0000FF;
//   //   p = convert(curve.control);
//   //   ((u32*)image.data())[(image.height - p.y - 1) * image.width + p.x] =
//   //       0xFF00FF00;
//   //   p = convert(curve.p1);
//   //   ((u32*)image.data())[(image.height - p.y - 1) * image.width + p.x] =
//   //       0xFFFF0000;
//   // }

//   // FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
//   // for (i32 y = 0; y < face->glyph->bitmap.rows; y++) {
//   //   for (i32 x = 0; x < face->glyph->bitmap.width; x++) {
//   //     ((u32*)image.data())[(image.height - y - 1) * image.width + x] =
//   //         0xFF000000 |
//   //         (face->glyph->bitmap.buffer[(face->glyph->bitmap.rows - y - 1) *
//   //                                         face->glyph->bitmap.width +
//   //                                     x]
//   //          << 8);
//   //   }
//   // }

//   return (f32)face->glyph->metrics.width / face->glyph->metrics.height;
// }