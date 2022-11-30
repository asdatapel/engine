#version 450

layout(binding = 0) uniform sampler2D tex_sampler;

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_color;
layout(location = 2) in float in_texture_blend_factor;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(in_color.rgb, mix(in_color.a, texture(tex_sampler, in_uv).r, in_texture_blend_factor));
}