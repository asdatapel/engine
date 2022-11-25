#version 450

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_color;

layout(location = 0) out vec3 out_color;

layout( push_constant ) uniform constants
{
    vec2 canvas_size;
} push_constants;

void main() {
    vec2 pos = in_position / push_constants.canvas_size * vec2(2, 2) - vec2(1, 1);
    gl_Position = vec4(pos, 0, 1);
    out_color = in_color;
}