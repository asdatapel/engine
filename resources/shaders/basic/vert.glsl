#version 450

layout(location = 0) in vec2  in_position;
layout(location = 1) in vec2  in_uv;
layout(location = 2) in vec4  in_color;
layout(location = 3) in float in_texture_blend_factor;
layout(location = 4) in vec2 in_corner;
layout(location = 5) in vec2 in_rect_center;

layout(location = 0) out vec2  out_uv;
layout(location = 1) out vec4  out_color;
layout(location = 2) out float out_texture_blend_factor;
layout(location = 3) out vec2 out_corner;
layout(location = 4) out vec2 out_position;
layout(location = 5) out vec2 out_rect_center;

layout( push_constant ) uniform constants
{
    vec2 canvas_size;
} push_constants;

void main() {
    vec2 pos = in_position / push_constants.canvas_size * vec2(2, 2) - vec2(1, 1);
    gl_Position = vec4(pos, 0, 1);

    out_uv = in_uv;
    out_color = in_color;
    out_texture_blend_factor = in_texture_blend_factor;
    out_corner = in_corner;
    out_position = in_position;
    out_rect_center = in_rect_center;
}