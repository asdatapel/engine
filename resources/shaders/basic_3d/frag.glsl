#version 450

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D tex_samplers[];

void main() {
    out_color = vec4(texture(tex_samplers[0], in_uv).rgb, 1);
}
