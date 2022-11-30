#version 450

layout(location = 0) in vec2  in_position;
layout(location = 1) in vec2  in_uv;
layout(location = 2) in vec4  in_color;
layout(location = 3) in float in_texture_blend_factor;

layout(location = 0) out vec2  out_uv;
layout(location = 1) out vec4  out_color;
layout(location = 2) out float out_texture_blend_factor;

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
}