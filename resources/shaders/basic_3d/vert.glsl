#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec2 out_uv;

layout( push_constant ) uniform constants
{
    mat4 mvp;
} push_constants;

void main() {
    out_normal = in_normal;
    out_uv = in_uv;
    gl_Position = push_constants.mvp * vec4(in_position.xyz, 1);
}