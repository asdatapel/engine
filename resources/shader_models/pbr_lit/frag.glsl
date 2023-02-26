#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_world_pos;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec2 in_uv;

layout(std140, set = 0, binding = 0) readonly buffer PerFrameData {
    float time;
} per_frame_data;

layout(std140, set = 1, binding = 0) readonly buffer PerPassData {
    vec3 camera_position;
} per_pass_data;


layout(std140, set = 2, binding = 0) readonly buffer PerShaderModelData {
    vec3 camera_position;
} per_shader_model_data;
layout(std140, set = 2, binding = 1) uniform sampler2D brdf_lut;

void output_node(vec3 albedo, vec3 normal, float metal, float roughness, vec3 emit, float ao, float alpha)
{
  out_color = vec4(albedo, alpha);
}