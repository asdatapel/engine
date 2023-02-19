#version 450

layout(location = 0) in vec3 in_normal;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D tex_samplers[];

void main() {
    out_color = vec4(in_normal, 1);
}
