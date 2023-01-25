#version 450

layout(binding = 0) uniform sampler2D tex_sampler;

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_color;
layout(location = 2) in float in_texture_blend_factor;
layout(location = 3) in vec2 in_corner;
layout(location = 4) in vec2 in_position;
layout(location = 5) in vec2 rect_center;

layout(location = 0) out vec4 out_color;


// https://www.warp.dev/blog/how-to-draw-styled-rectangles-using-the-gpu-and-metal
float distance_from_rect(vec2 pixel_pos, vec2 rect_center, vec2 corner, float corner_radius) {
   vec2 p = pixel_pos - rect_center;
   vec2 q = abs(p) - (corner - vec2(corner_radius, corner_radius));
   return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - corner_radius;
}

void main() {
    out_color = vec4(in_color.rgb, mix(in_color.a, texture(tex_sampler, in_uv).r, in_texture_blend_factor));
    
    float dist = distance_from_rect(in_position, rect_center, in_corner, 3);
    float color = 1 - smoothstep(-1, 0, dist / 2);
    out_color.a = color * out_color.a;

}
