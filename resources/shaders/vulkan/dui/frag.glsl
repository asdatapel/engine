#version 450

#extension GL_EXT_nonuniform_qualifier : enable

#include "primitives.glsl"

layout(set = 0, binding = 0) uniform sampler2D tex_samplers[];

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_rect_positive_extents;
layout(location = 3) in vec2 in_position;
layout(location = 4) in vec2 in_rect_center;
layout(location = 5) in float in_rect_corner_radius;
layout(location = 6) in vec4 in_clip_rect_bounds;
layout(location = 7) in flat uint in_primitive_type;
layout(location = 8) in flat uint in_primitive_idx;
layout(location = 9) in flat uint in_texture_idx;

layout(location = 0) out vec4 out_color;


// https://www.warp.dev/blog/how-to-draw-styled-rectangles-using-the-gpu-and-metal
float distance_from_rect(vec2 pixel_pos, vec2 rect_center, vec2 corner, float corner_radius) {
   vec2 p = pixel_pos - rect_center;
   vec2 q = abs(p) - (corner - vec2(corner_radius, corner_radius));
   return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - corner_radius;
}

float clip(vec2 position, vec4 clip_rect_bounds) {
    vec2 axes_in_bounds = step(clip_rect_bounds.xy, position) * step(position, clip_rect_bounds.zw);
    return axes_in_bounds.x * axes_in_bounds.y;
}

vec2 rotate(vec2 p, float cos_theta, float sin_theta) {
  vec2 p2;
  p2.x = p.x * cos_theta - p.y * sin_theta;
  p2.y = p.x * sin_theta + p.y * cos_theta;
  return p2;
}

// copied from https://github.com/GreenLightning/gpu-font-rendering
float compute_coverage(ConicCurvePrimitive curve, vec2 origin, float rotation, float pixel_width)
{
  float cos_theta = cos(rotation);
  float sin_theta = sin(rotation);

  curve.p0 -= origin;
  curve.p1 -= origin;
  curve.p2 -= origin;

  curve.p0 = rotate(curve.p0, cos_theta, sin_theta);
  curve.p1 = rotate(curve.p1, cos_theta, sin_theta);
  curve.p2 = rotate(curve.p2, cos_theta, sin_theta);

  vec2 p0 = curve.p0;
  vec2 p1 = curve.p1;
  vec2 p2 = curve.p2;

  if (p0.y >= 0 && p1.y >= 0 && p2.y >= 0) return 0.0;
  if (p0.y < 0 && p1.y < 0 && p2.y < 0) return 0.0;

  // Note: Simplified from abc formula by extracting a factor of (-2) from b.
  vec2 a = p0 - 2 * p1 + p2;
  vec2 b = p0 - p1;
  vec2 c = p0;

  float t0, t1;
  if (abs(a.y) >= 1e-5) {
    float radicand = b.y * b.y - a.y * c.y;
    if (radicand <= 0) return 0.0;

    float s = sqrt(radicand);
    t0      = (b.y - s) / a.y;
    t1      = (b.y + s) / a.y;
  } else {
    float t = p0.y / (p0.y - p2.y);
    if (p0.y < p2.y) {
      t0 = -1.0;
      t1 = t;
    } else {
      t0 = t;
      t1 = -1.0;
    }
  }

  float alpha = 0;

  if (t0 >= 0 && t0 < 1) {
    float x = (a.x * t0 - 2.0 * b.x) * t0 + c.x;
    alpha += clamp(x * (1.f / pixel_width) + .5f, 0.f, 1.f);
  }

  if (t1 >= 0 && t1 < 1) {
    float x = (a.x * t1 - 2.0 * b.x) * t1 + c.x;
    alpha -= clamp(x * (1.f / pixel_width) + .5f, 0.f, 1.f);
  }

  return alpha;
}

void main() {
    if (in_primitive_type == ROUNDED_RECT) {
      out_color = in_color;
      
      if (in_rect_corner_radius > 0) {
          float dist = distance_from_rect(in_position, in_rect_center, in_rect_positive_extents, in_rect_corner_radius);
          float aa = 1 - smoothstep(-1, 0, dist);
          out_color.a = aa * out_color.a;
      }

      out_color.a *= clip(in_position, in_clip_rect_bounds);
    } else if (in_primitive_type == TEXTURE_RECT) {
      out_color = texture(tex_samplers[in_texture_idx], in_uv);
      out_color.a *= clip(in_position, in_clip_rect_bounds);
      //out_color = vec4(1, 0, 0, 1);
    } else if (in_primitive_type == BITMAP_GLYPH) {
      out_color = vec4(in_color.rgb, in_color.a * texture(tex_samplers[0], in_uv).r);
      out_color.a *= clip(in_position, in_clip_rect_bounds);
    } else if (in_primitive_type == VECTOR_GLYPH) {
      VectorGlyphPrimitive glyph = primitives.vector_glyphs[in_primitive_idx];

      int n_samples = 8;
      float coverage  = 0.f;
      for (int curve_i = 0; curve_i < glyph.curve_count; curve_i++) {
        ConicCurvePrimitive curve = primitives.conic_curves[glyph.curve_start_idx + curve_i];
        for (int s = 0; s < n_samples; s++) {
          float angle = 3.1415 * s / n_samples;
          float width = length(fwidth(in_uv) * vec2(cos(angle), sin(angle))) * 2;
          coverage += compute_coverage(curve, in_uv, angle, width) /
                      n_samples;
        }
      }
      out_color = vec4(in_color.rgb, in_color.a * coverage);
      out_color.a *= clip(in_position, in_clip_rect_bounds);
    } else if (in_primitive_type == LINE) {
      LinePrimitive line = primitives.lines[in_primitive_idx];

      // ACK: https://iquilezles.org/articles/distfunctions2d/
      vec2 pa    = in_position - line.a;
      vec2 ba    = line.b - line.a;
      float h    = clamp(dot(pa, ba) / dot(ba, ba), 0, 1);
      float dist = length(pa - h * ba);

      float aa = smoothstep(0, 2, dist);
      out_color = in_color;
      out_color.a = (1 - aa) * out_color.a;

      out_color.a *= clip(in_position, in_clip_rect_bounds);
    }
}
