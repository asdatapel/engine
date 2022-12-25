#pragma once

#include <algorithm>
#include <execution>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "input.hpp"
#include "logging.hpp"

FT_Library library;
FT_Face face;

struct ConicCurve {
  Vec2f p0, p1, control;
};

template <u32 WIDTH, u32 HEIGHT>
struct Grid {
  struct Cell {
    Array<ConicCurve, 32> curves;
    b8 inside = false;
  };

  const u32 width  = WIDTH;
  const u32 height = HEIGHT;

  Cell cells[WIDTH * HEIGHT];
  Cell& get(u32 x, u32 y) { return cells[y * HEIGHT + x]; }
};

const Vec2i grid_dimensions = Vec2i(16, 16);
const Vec2f cell_size =
    Vec2f(1, 1) / Vec2f(grid_dimensions.x, grid_dimensions.y);
Grid<16, 16> grid;

void rasterize(Image image, Input* input)
{
  static b8 init = false;
  if (!init) {
    init         = true;
    FT_Error err = FT_Init_FreeType(&library);
    if (err) {
      fatal("failed to init freetype");
    }
    err =
        FT_New_Face(library, "resources/fonts/OpenSans-Regular.ttf", 0, &face);
    if (err) {
      fatal("failed to load font");
    }
  }

  static char c = 'C';
  if (input->text_input.size > 0) {
    c = input->text_input[0];
  }

  FT_Error err    = FT_Set_Pixel_Sizes(face, /* handle to face object */
                                       0,    /* pixel_width           */
                                       1);   /* pixel_height          */
  u32 glyph_index = FT_Get_Char_Index(face, c);
  err             = FT_Load_Glyph(face,             /* handle to face object */
                                  glyph_index,      /* glyph index           */
                                  FT_LOAD_DEFAULT); /* load flags, see below */

  if (face->glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
    fatal("no glyph outline");
  }
  FT_Outline outline = face->glyph->outline;

  Array<ConicCurve, 256> curves;

  for (i32 contour_i = 0; contour_i < outline.n_contours; contour_i++) {
    i32 contour_start =
        contour_i == 0 ? 0 : outline.contours[contour_i - 1] + 1;
    i32 contour_end = outline.contours[contour_i];

    b8 on_ghost_point = false;
    Vec2f ghost_point;

    i32 point_i = contour_start;
    while (point_i <= contour_end) {
      if (FT_CURVE_TAG(outline.tags[point_i]) == FT_CURVE_TAG_ON)
        printf("ON, ");
      if (FT_CURVE_TAG(outline.tags[point_i]) == FT_CURVE_TAG_CONIC)
        printf("CONIC, ");
      if (FT_CURVE_TAG(outline.tags[point_i]) == FT_CURVE_TAG_CUBIC)
        printf("CUBIC, ");

      point_i++;
    }

    // add to grid struct
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
                  face->glyph->metrics.width;
          f32 y = ((f32)ft_vec.y + (face->glyph->metrics.height -
                                    face->glyph->metrics.horiBearingY)) /
                  face->glyph->metrics.height;

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
          curves.push_back({a, b, (a + b) / Vec2f(2.f, 2.f)});
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

          curves.push_back({a, c, b});

          point_i++;
        }
      }
    }
  }

  struct Roots {
    i8 num_roots = 0;
    f32 t0;
    f32 t1;

    f32 x0;
    f32 x1;

    b8 completely_below;
  };
  auto get_roots = [](ConicCurve curve, Vec2f origin, f32 rotation) {
    Roots roots;

    f32 cos_theta = cosf(rotation);
    f32 sin_theta = sinf(rotation);

    auto rotate = [&](Vec2f p) {
      Vec2f p2;
      p2.x = p.x * cos_theta - p.y * sin_theta;
      p2.y = p.x * sin_theta + p.y * cos_theta;
      return p2;
    };

    curve.p0 -= origin;
    curve.p1 -= origin;
    curve.control -= origin;

    curve.p0      = rotate(curve.p0);
    curve.p1      = rotate(curve.p1);
    curve.control = rotate(curve.control);

    roots.completely_below =
        (curve.p0.y <= 0 && curve.p1.y <= 0 && curve.control.y <= 0);

    Vec2f a = curve.p0 - (2 * curve.control) + curve.p1;
    Vec2f b = (2 * curve.control) - (2 * curve.p0);
    Vec2f c = curve.p0;

    // straight line
    if (fabs(a.y) < 0.0000005f) {
      roots.t0 = (-c.y) / b.y;
      roots.x0 = (b.x * roots.t0) + c.x;
      roots.num_roots++;
      return roots;
    };

    f32 det = (b.y * b.y) - (4 * a.y * c.y);
    // if (fabsf(det) < 0.00001f) return roots;

    roots.t0 = (-b.y + sqrt(det)) / (2 * a.y);
    roots.x0 = (a.x * roots.t0 * roots.t0) + (b.x * roots.t0) + c.x;
    roots.num_roots++;

    if (fabsf(det) < 0.00001f) return roots;

    roots.t1 = (-b.y - sqrt(det)) / (2 * a.y);
    roots.x1 = (a.x * roots.t1 * roots.t1) + (b.x * roots.t1) + c.x;
    roots.num_roots++;

    return roots;
  };

  // determine whether each grid cell is inside
  // for (i32 y = 0; y < grid.height; y++) {
  //   for (i32 x = 0; x < grid.width; x++) {
  //     Vec2f cell_size   = Vec2f(1, 1) / Vec2f(grid.width, grid.height);
  //     Vec2f cell_center = cell_size * Vec2f(x + .5f, y + .5f);

  //     int DEBUG_Y = 51;
  //     if (y == DEBUG_Y) {
  //       y = y;
  //     }

  //     b8 inside = false;
  //     i8 wind   = 0;
  //     for (i32 i = 0; i < curves.size; i++) {
  //       if (y == DEBUG_Y && x == 0) {
  //         printf("%i: ", i);

  //         if (i == 22) {
  //           printf("asdas");
  //         }
  //       }
  //       Roots roots = get_roots(curves[i], cell_center, 0);

  //       if (roots.num_roots > 0 && roots.t0 >= 0 && roots.t0 < 1 &&
  //           roots.x0 > 0) {
  //         inside = !inside;

  //         if (y == DEBUG_Y && x == 0) {
  //           printf("t0 %f", roots.t0);
  //         }
  //       }
  //       if (roots.num_roots > 1 && roots.t1 >= 0 && roots.t1 < 1 &&
  //           roots.x1 > 0) {
  //         inside = !inside;
  //         if (y == DEBUG_Y && x == 0) {
  //           printf(" t1 %f", roots.t1);
  //         }
  //       }

  //       if (y == DEBUG_Y && x == 0) {
  //         printf(", ");
  //       }
  //     }

  //     // if (y == DEBUG_Y) inside = true;

  //     grid.get(x, y).inside = inside;
  //   }
  // }
  // printf("\n");

  // scanline for grid cell centers in or out
  for (i32 y = 0; y < grid.height; y++) {
    f32 cell_center_y = (y + .5f) * (1.f / grid.height);

    Array<f32, 32> sorted_curves;

    for (i32 i = 0; i < curves.size; i++) {
      Roots roots = get_roots(curves[i], Vec2f(0, cell_center_y), 0);

      if (roots.num_roots > 0 && !roots.completely_below && roots.t0 >= 0 &&
          roots.t0 <= 1 && roots.x0 > .0f) {
        f32 x = roots.x0;

        i32 index = 0;
        while (index < sorted_curves.size && sorted_curves[index] < x) {
          index++;
        }
        sorted_curves.insert(index, x);
      }

      if (roots.num_roots > 1 && !roots.completely_below && roots.t1 >= 0 &&
          roots.t1 <= 1 && roots.x1 > .0f) {
        f32 x = roots.x1;

        i32 index = 0;
        while (index < sorted_curves.size && sorted_curves[index] < x) {
          index++;
        }

        sorted_curves.insert(index, x);
      }
    }

    sorted_curves.push_back(1);

    f32 x = 0.f;
    b8 in = false;
    for (i32 i = 0; i < sorted_curves.size; i++) {
      i32 grid_x_min = ceilf(x * grid.width - .5f);
      i32 grid_x_max = ceilf(sorted_curves[i] * grid.width - .5f);

      for (i32 i = grid_x_min; i < grid_x_max; i++) {
        grid.get(i, y).inside = in;
      }

      in = !in;
      x  = sorted_curves[i];
    }

    x  = 0.f;
    in = false;
    while (x < 1.f) {
      f32 min_hit_x = INFINITY;

      for (i32 i = 0; i < curves.size; i++) {
        Roots roots = get_roots(curves[i], Vec2f(x, cell_center_y), 0);

        if (roots.num_roots > 0 && !roots.completely_below && roots.t0 >= 0 &&
            roots.t0 <= 1 && roots.x0 > .0000001f) {
          min_hit_x = fminf(min_hit_x, x + roots.x0);
        }
        if (roots.num_roots > 1 && !roots.completely_below && roots.t1 >= 0 &&
            roots.t1 <= 1 && roots.x1 > .0000001f) {
          min_hit_x = fminf(min_hit_x, x + roots.x1);
        }
      }

      i32 grid_x_min = ceilf(x * grid.width - .5f);
      i32 grid_x_max = grid.width - 1;
      if (min_hit_x != INFINITY) {
        grid_x_max = fminf(grid_x_max, ceilf(min_hit_x * grid.width - .5f));
      }

      for (i32 i = grid_x_min; i < grid_x_max; i++) {
        grid.get(i, y).inside = in;
      }

      in = !in;
      x  = min_hit_x;
    }
  }

  auto does_intersect = [&](ConicCurve curve, Vec2f origin, f32 rotation,
                            Vec2f x_range = {-1.f, 1.f}) {
    Roots roots = get_roots(curve, origin, rotation);

    if (roots.num_roots > 0 && !roots.completely_below) {
      if (roots.t0 >= 0 && roots.t0 <= 1 && roots.x0 >= x_range.x &&
          roots.x0 <= x_range.y) {
        return true;
      }
    }

    if (roots.num_roots > 1 && !roots.completely_below) {
      if (roots.t1 >= 0 && roots.t1 <= 1 && roots.x1 >= x_range.x &&
          roots.x1 <= x_range.y) {
        return true;
      }
    }

    return false;
  };

  for (i32 grid_y = 0; grid_y < grid_dimensions.y; grid_y++) {
    for (i32 grid_x = 0; grid_x < grid_dimensions.x; grid_x++) {
      grid.get(grid_x, grid_y).curves.clear();

      Vec2f top_left     = Vec2f(grid_x, grid_y) * cell_size;
      Vec2f top_right    = Vec2f(grid_x + 1, grid_y) * cell_size;
      Vec2f bottom_left  = Vec2f(grid_x, grid_y + 1) * cell_size;
      Vec2f bottom_right = Vec2f(grid_x + 1, grid_y + 1) * cell_size;

      for (i32 curve_i = 0; curve_i < curves.size; curve_i++) {
        ConicCurve curve = curves[curve_i];

        b8 top_intersects =
            does_intersect(curve, top_left, 0, {0, cell_size.x});
        b8 bottom_intersects =
            does_intersect(curve, bottom_left, 0, {0, cell_size.x});
        b8 left_intersects =
            does_intersect(curve, top_left, -3.1415f / 2.f, {0, cell_size.x});
        b8 right_intersects =
            does_intersect(curve, top_right, -3.1415f / 2.f, {0, cell_size.x});
        b8 starts_in_rect = in_rect(
            curve.p0, {top_left.x, top_left.y, cell_size.x, cell_size.y});

        if (starts_in_rect || top_intersects || bottom_intersects ||
            left_intersects || right_intersects) {
          grid.get(grid_x, grid_y).curves.push_back(curve);
        }
      }
    }
  }

  auto IntersectHorz = [](ConicCurve curve, float Y, float outX[2]) {
    Vec2f A = curve.p0;
    Vec2f B = curve.control;
    Vec2f C = curve.p1;
    int i   = 0;

#define T_VALID(t) ((t) <= 1 && (t) >= 0)
#define X_FROM_T(t) \
  ((1 - (t)) * (1 - (t)) * A.x + 2 * (t) * (1 - (t)) * B.x + (t) * (t)*C.x)

    // Parts of the bezier function solved for t
    float a = A.y - 2 * B.y + C.y;

    // In the condition that a=0, the standard formulas won't work
    if (std::fabs(a) < 1e-5) {
      float t = (2 * B.y - C.y - Y) / (2 * (B.y - C.y));
      if (T_VALID(t)) {
        outX[i++] = X_FROM_T(t);
      }
      return i;
    }

    float sqrtTerm = std::sqrt(Y * a + B.y * B.y - A.y * C.y);

    float t = (A.y - B.y + sqrtTerm) / a;
    if (T_VALID(t)) {
      outX[i++] = X_FROM_T(t);
    }

    t = (A.y - B.y - sqrtTerm) / a;
    if (T_VALID(t)) {
      outX[i++] = X_FROM_T(t);
    }

    return i;

#undef X_FROM_T
#undef T_VALID
  };

  i32 grid_cell_size_x = image.width / grid.width;
  i32 grid_cell_size_y = image.height / grid.height;

  for (i32 y = 0; y < image.height; y++) {
    for (i32 x = 0; x < image.width; x++) {
      i32 grid_cell_x = (f32)x / image.width * grid_dimensions.x;
      i32 grid_cell_y = (f32)y / image.height * grid_dimensions.y;
      Array<ConicCurve, 32>& cell_curves =
          grid.get(grid_cell_x, grid_cell_y).curves;

      Vec2f pixel_center =
          Vec2f((x + .5f) / image.width, (y + .5f) / image.height);

      u32 hits = 0;
      for (i32 curve_i = 0; curve_i < cell_curves.size; curve_i++) {
        u32 this_curve_hits = 0;
        for (i32 s = 0; s < 8; s++) {
          if (does_intersect(cell_curves[curve_i], pixel_center,
                             s * 2 * (3.1415 / 8),
                             Vec2f(-1.f / image.width, 1.f / image.width))) {
            this_curve_hits++;
          }
        }

        hits = std::max(hits, this_curve_hits);
      }

      u32 inside  = grid.get(grid_cell_x, grid_cell_y).inside ? 255 : 0;
      u32 on_grid = (x % grid_cell_size_x < 3) || (y % grid_cell_size_y < 3)
                        ? 0x00FF0000
                        : 0;
      ((u32*)image.data())[(image.height - y - 1) * image.width + x] =
          0xFF000000 | ((hits * 16) << 8) | inside | on_grid;
    }
  }

  for (i32 i = 0; i < curves.size; i++) {
    ConicCurve curve = curves[i];

    auto convert = [&](Vec2f p) {
      return Vec2i(p.x * image.width, p.y * image.height);
    };
    Vec2i p = convert(curve.p0);
    ((u32*)image.data())[(image.height - p.y - 1) * image.width + p.x] =
        0xFF0000FF;
    p = convert(curve.control);
    ((u32*)image.data())[(image.height - p.y - 1) * image.width + p.x] =
        0xFF00FF00;
    p = convert(curve.p1);
    ((u32*)image.data())[(image.height - p.y - 1) * image.width + p.x] =
        0xFFFF0000;
  }
}