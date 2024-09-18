#include <metal_stdlib>

#include "../basic.metal"

constant u32 RECT         = 1 << 18;
constant u32 ROUNDED_RECT = 2 << 18;
constant u32 TEXTURE_RECT = 3 << 18;
constant u32 BITMAP_GLYPH = 4 << 18;
constant u32 VECTOR_GLYPH = 5 << 18;
constant u32 LINE         = 6 << 18;

struct RectPrimitive {
  Vec4f rect;
};

struct RoundedRectPrimitive {
  Vec4f dimensions;
  u32 clip_rect_idx;
  u32 color;
  f32 corner_radius;
  u32 corner_mask;
};

struct TextureRectPrimitive {
  Vec4f dimensions;
  Vec4f uv_bounds;
  u32 texture_idx;
  u32 clip_rect_idx;
};

struct BitmapGlyphPrimitive {
  Vec4f dimensions;
  Vec4f uv_bounds;
  u32 clip_rect_idx;
  u32 color;
};

struct ConicCurvePrimitive {
  Vec2f p0;
  Vec2f p1;
  Vec2f p2;
};

struct VectorGlyphPrimitive {
  Vec4f dimensions;
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
};

struct Primitives {
  RectPrimitive clip_rects[1024];
  RoundedRectPrimitive rounded_rects[1024];
  BitmapGlyphPrimitive bitmap_glyphs[1024];
  TextureRectPrimitive texture_rects[1024];
  VectorGlyphPrimitive vector_glyphs[1024];
  ConicCurvePrimitive conic_curves[4096];
  LinePrimitive lines[4096];
  packed_float4 canvas_size;
};

struct Args {
    device Primitives *primitives;
};

struct VertexOut {
    float4 ndc_position [[position]];
    float2 position;
    float2 uv;
    float4 color;
    float2 rect_positive_extents;
    float2 rect_center;
    f32 rect_corner_radius;
    float4 clip_rect_bounds;
    u32 primitive_type [[flat]];
    u32 primitive_idx [[flat]];
    u32 texture_idx [[flat]];
};

Vec4f uint_to_vec_color(u32 color) {
    f32 r = ((color >> 24) & 0xFF) / 255.0;
    f32 g = ((color >> 16) & 0xFF) / 255.0;
    f32 b = ((color >> 8) & 0xFF) / 255.0;
    f32 a = ((color >> 0) & 0xFF) / 255.0;

    return Vec4f(r, g, b, a);
}

vertex VertexOut
vertex_shader(u32 vertex_id [[vertex_id]],
              constant Args *args)
{
    VertexOut out;
    device Primitives *primitives = args->primitives;

    u32 primitive_idx = vertex_id & 0xFFFF;

    out.primitive_type = vertex_id & 0xFFFC0000;
    out.primitive_idx = primitive_idx;

    if (out.primitive_type == ROUNDED_RECT) {
        RoundedRectPrimitive r = primitives->rounded_rects[primitive_idx];
        RectPrimitive clip = primitives->clip_rects[r.clip_rect_idx];

        u32 corner = (vertex_id >> 16) & 0x3;
        float2 verts[] = {
            {r.dimensions.x,  r.dimensions.y},
            {r.dimensions.x + r.dimensions.z,  r.dimensions.y},
            {r.dimensions.x,  r.dimensions.y + r.dimensions.w},
            {r.dimensions.x + r.dimensions.z,  r.dimensions.y + r.dimensions.w},
        };

        out.ndc_position = float4(verts[corner] / primitives->canvas_size.xy * float2(2, 2) - float2(1, 1), 0, 1);

        float2 rect_positive_extents = r.dimensions.zw / 2;
        float2 rect_center = r.dimensions.xy + r.dimensions.zw / 2;

        out.color = uint_to_vec_color(r.color);
        out.rect_positive_extents = rect_positive_extents;
        out.position = verts[corner];
        out.rect_center = rect_center;
        out.rect_corner_radius = r.corner_radius * metal::smoothstep(0.f, 1.f, r.corner_mask & (1 << corner));
        out.clip_rect_bounds = float4(clip.rect.x, clip.rect.y, clip.rect.x + clip.rect.z, clip.rect.y + clip.rect.w);
    } else if (out.primitive_type == TEXTURE_RECT) {
        TextureRectPrimitive p = primitives->texture_rects[primitive_idx];
        RectPrimitive clip = primitives->clip_rects[p.clip_rect_idx];

        u32 corner = (vertex_id >> 16) & 0x3;
        float2 verts[] = {
            {p.dimensions.x,  p.dimensions.y},
            {p.dimensions.x + p.dimensions.z,  p.dimensions.y},
            {p.dimensions.x,  p.dimensions.y + p.dimensions.w},
            {p.dimensions.x + p.dimensions.z,  p.dimensions.y + p.dimensions.w},
        };
        float2 uvs[] = {
            {p.uv_bounds.x, p.uv_bounds.y},
            {p.uv_bounds.z, p.uv_bounds.y},
            {p.uv_bounds.x, p.uv_bounds.w},
            {p.uv_bounds.z, p.uv_bounds.w},
        };

        out.ndc_position = float4(verts[corner] / primitives->canvas_size.xy * float2(2, 2) - float2(1, 1), 0, 1);

        out.uv = uvs[corner];
        out.position = verts[corner];
        out.texture_idx = p.texture_idx;
        out.clip_rect_bounds = float4(clip.rect.x, clip.rect.y, clip.rect.x + clip.rect.z, clip.rect.y + clip.rect.w);
    } else if (out.primitive_type == BITMAP_GLYPH) {
        BitmapGlyphPrimitive p = primitives->bitmap_glyphs[primitive_idx];
        RectPrimitive clip = primitives->clip_rects[p.clip_rect_idx];

        u32 corner = (vertex_id >> 16) & 0x3;
        float2 verts[] = {
            {p.dimensions.x, p.dimensions.y},
            {p.dimensions.x + p.dimensions.z, p.dimensions.y},
            {p.dimensions.x,  p.dimensions.y + p.dimensions.w},
            {p.dimensions.x + p.dimensions.z,  p.dimensions.y + p.dimensions.w},
        };
        float2 uvs[] = {
            {p.uv_bounds.x, p.uv_bounds.y},
            {p.uv_bounds.z, p.uv_bounds.y},
            {p.uv_bounds.x,  p.uv_bounds.w},
            {p.uv_bounds.z,  p.uv_bounds.w},
        };

        out.ndc_position = float4(verts[corner] / primitives->canvas_size.xy * float2(2, 2) - float2(1, 1), 0, 1);

        out.uv = uvs[corner];
        out.color = uint_to_vec_color(p.color);
        out.position = verts[corner];
        out.clip_rect_bounds = float4(clip.rect.x, clip.rect.y, clip.rect.x + clip.rect.z, clip.rect.y + clip.rect.w);
    } else if (out.primitive_type == VECTOR_GLYPH) {
        VectorGlyphPrimitive p = primitives->vector_glyphs[primitive_idx];
        RectPrimitive clip = primitives->clip_rects[p.clip_rect_idx];

        u32 corner = (vertex_id >> 16) & 0x3;
        float2 verts[] = {
            {p.dimensions.x - 1 , p.dimensions.y - 1},
            {p.dimensions.x + p.dimensions.z + 1, p.dimensions.y - 1},
            {p.dimensions.x - 1,  p.dimensions.y + p.dimensions.w + 1},
            {p.dimensions.x + p.dimensions.z + 1,  p.dimensions.y + p.dimensions.w + 1},
        };
        float2 uvs[] = {
            {-(1 / p.dimensions.z),       1 + (1 / p.dimensions.w)},
            {1 + (1 / p.dimensions.z),   1 + (1 / p.dimensions.w)},
            {-(1 / p.dimensions.z),     -(1 / p.dimensions.w)},
            {1 + (1 / p.dimensions.z), -(1 / p.dimensions.w)},
        };

        out.ndc_position = float4(verts[corner] / primitives->canvas_size.xy * float2(2, 2) - float2(1, 1), 0, 1);

        out.uv = uvs[corner];
        out.color = uint_to_vec_color(p.color);
        out.position = verts[corner];
        out.clip_rect_bounds = float4(clip.rect.x, clip.rect.y, clip.rect.x + clip.rect.z, clip.rect.y + clip.rect.w);
    } else if (out.primitive_type == LINE) {
        LinePrimitive p = primitives->lines[primitive_idx];
        RectPrimitive clip = primitives->clip_rects[p.clip_rect_idx];

        float2 d = metal::normalize(p.b - p.a);
        float2 tangent = float2(-d.y, d.x) * 10;

        u32 corner = (vertex_id >> 16) & 0x3;
        float2 verts[] = {
            p.a + tangent,
            p.a - tangent,
            p.b + tangent,
            p.b - tangent,
        };
        out.ndc_position = float4(verts[corner] / primitives->canvas_size.xy * float2(2, 2) - float2(1, 1), 0, 1);

        out.color = uint_to_vec_color(p.color);
        out.position = verts[corner];
        out.clip_rect_bounds = float4(clip.rect.x, clip.rect.y, clip.rect.x + clip.rect.z, clip.rect.y + clip.rect.w);
    }

    out.ndc_position.y *= -1;
    return out;
}

f32 distance_from_rect(Vec2f pixel_pos, Vec2f rect_center, Vec2f corner, f32 corner_radius) {
   Vec2f p = pixel_pos - rect_center;
   Vec2f q = metal::abs(p) - (corner - Vec2f(corner_radius, corner_radius));
   return metal::length(metal::max(q, 0.0)) + metal::min(metal::max(q.x, q.y), 0.0) - corner_radius;
}

f32 clip(Vec2f position, Vec4f clip_rect_bounds) {
    Vec2f axes_in_bounds = metal::step(clip_rect_bounds.xy, position) * metal::step(position, clip_rect_bounds.zw);
    return axes_in_bounds.x * axes_in_bounds.y;
}

Vec2f rotate(Vec2f p, f32 cos_theta, f32 sin_theta) {
  Vec2f p2;
  p2.x = p.x * cos_theta - p.y * sin_theta;
  p2.y = p.x * sin_theta + p.y * cos_theta;
  return p2;
}

// copied from https://github.com/GreenLightning/gpu-font-rendering
f32 compute_coverage(ConicCurvePrimitive curve, Vec2f origin, f32 rotation, f32 pixel_width)
{
  f32 cos_theta = metal::cos(rotation);
  f32 sin_theta = metal::sin(rotation);

  curve.p0 -= origin;
  curve.p1 -= origin;
  curve.p2 -= origin;

  curve.p0 = rotate(curve.p0, cos_theta, sin_theta);
  curve.p1 = rotate(curve.p1, cos_theta, sin_theta);
  curve.p2 = rotate(curve.p2, cos_theta, sin_theta);

  Vec2f p0 = curve.p0;
  Vec2f p1 = curve.p1;
  Vec2f p2 = curve.p2;

  if (p0.y >= 0 && p1.y >= 0 && p2.y >= 0) return 0.0;
  if (p0.y < 0 && p1.y < 0 && p2.y < 0) return 0.0;

  // Note: Simplified from abc formula by extracting a factor of (-2) from b.
  Vec2f a = p0 - 2 * p1 + p2;
  Vec2f b = p0 - p1;
  Vec2f c = p0;

  f32 t0, t1;
  if (metal::abs(a.y) >= 1e-5) {
    f32 radicand = b.y * b.y - a.y * c.y;
    if (radicand <= 0) return 0.0;

    f32 s = metal::sqrt(radicand);
    t0      = (b.y - s) / a.y;
    t1      = (b.y + s) / a.y;
  } else {
    f32 t = p0.y / (p0.y - p2.y);
    if (p0.y < p2.y) {
      t0 = -1.0;
      t1 = t;
    } else {
      t0 = t;
      t1 = -1.0;
    }
  }

  f32 alpha = 0;

  if (t0 >= 0 && t0 < 1) {
    f32 x = (a.x * t0 - 2.0 * b.x) * t0 + c.x;
    alpha += metal::clamp(x * (1.f / pixel_width) + .5f, 0.f, 1.f);
  }

  if (t1 >= 0 && t1 < 1) {
    f32 x = (a.x * t1 - 2.0 * b.x) * t1 + c.x;
    alpha -= metal::clamp(x * (1.f / pixel_width) + .5f, 0.f, 1.f);
  }

  return alpha;
}

fragment float4 fragment_shader(VertexOut in [[stage_in]], constant Args *args) {
    Vec4f out(0);

    device Primitives *primitives = args->primitives;
    if (in.primitive_type == ROUNDED_RECT) {
      out = in.color;
      
      if (in.rect_corner_radius > 0) {
          f32 dist = distance_from_rect(in.position, in.rect_center, in.rect_positive_extents, in.rect_corner_radius);
          f32 aa = 1 - metal::smoothstep(-1, 0, dist);
          out.a = aa * out.a;
      }

      out.a *= clip(in.position, in.clip_rect_bounds);
    } else if (in.primitive_type == TEXTURE_RECT) {
      // out = texture(tex_samplers[in.texture_idx], in.uv);
      out.a *= clip(in.position, in.clip_rect_bounds);
    } else if (in.primitive_type == BITMAP_GLYPH) {
      // out = Vec4f(in.color.rgb, in.color.a * texture(tex_samplers[0], in.uv).r);
      out.a *= clip(in.position, in.clip_rect_bounds);
    } else if (in.primitive_type == VECTOR_GLYPH) {
      VectorGlyphPrimitive glyph = primitives->vector_glyphs[in.primitive_idx];

      u32 n_samples = 8;
      f32 coverage  = 0.f;
      for (u32 curve_i = 0; curve_i < glyph.curve_count; curve_i++) {
        ConicCurvePrimitive curve = primitives->conic_curves[glyph.curve_start_idx + curve_i];
        for (u32 s = 0; s < n_samples; s++) {
          f32 angle = 3.1415 * s / n_samples;
          f32 width = metal::length(metal::fwidth(in.uv) * Vec2f(metal::cos(angle), metal::sin(angle))) * 2;
          coverage += compute_coverage(curve, in.uv, angle, width) /
                      n_samples;
        }
      }
      out = Vec4f(in.color.rgb, in.color.a * coverage);
      out.a *= clip(in.position, in.clip_rect_bounds);
    } else if (in.primitive_type == LINE) {
      LinePrimitive line = primitives->lines[in.primitive_idx];

      // ACK: https://iquilezles.org/articles/distfunctions2d/
      Vec2f pa    = in.position - line.a;
      Vec2f ba    = line.b - line.a;
      f32 h    = metal::clamp(metal::dot(pa, ba) / metal::dot(ba, ba), 0.f, 1.f);
      f32 dist = metal::length(pa - h * ba);

      f32 aa = metal::smoothstep(0, 2, dist);
      out = in.color;
      out.a = (1 - aa) * out.a;

      out.a *= clip(in.position, in.clip_rect_bounds);
    }

    return out;
}