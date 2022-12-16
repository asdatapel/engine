#version 450

#include "primitives.glsl"

layout(binding = 0) uniform sampler2D tex_sampler;

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_color;
layout(location = 2) in float in_texture_blend_factor;
layout(location = 3) in vec2 in_rect_positive_extents;
layout(location = 4) in vec2 in_position;
layout(location = 5) in vec2 in_rect_center;
layout(location = 6) in float in_rect_corner_radius;
layout(location = 7) in vec4 in_clip_rect_bounds;
layout(location = 8) in flat uint in_primitive_type;

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
//      out_color.a = mix(0, out_color.a, both_in_bounds);
}

void main() {
    if (in_primitive_type == ROUNDED_RECT) {
      out_color = in_color;
      
      if (in_rect_corner_radius > 0) {
          float dist = distance_from_rect(in_position, in_rect_center, in_rect_positive_extents, in_rect_corner_radius);
          float color = 1 - smoothstep(-1, 0, dist / 2);
          out_color.a = color * out_color.a;
      }

      //vec2 axes_in_bounds = step(in_clip_rect_bounds.xy, in_position) * step(in_position, in_clip_rect_bounds.zw);
      //float both_in_bounds = axes_in_bounds.x * axes_in_bounds.y;
      //out_color.a = mix(0, out_color.a, both_in_bounds);
      out_color.a *= clip(in_position, in_clip_rect_bounds);
    }
    if (in_primitive_type == BITMAP_GLYPH) {
      out_color = vec4(in_color.rgb, in_color.a * texture(tex_sampler, in_uv).r);
      out_color.a *= clip(in_position, in_clip_rect_bounds);
    }
}
