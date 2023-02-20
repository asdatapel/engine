#pragma once

#include "containers/array.hpp"
#include "model_import.hpp"
#include "string.hpp"

namespace Editor
{

struct UploadedMesh {
  String filename;
  Gpu::Buffer vertex_buffer;
  Gpu::Buffer index_buffer;

  VkDescriptorSet desc_set;
  VkSampler sampler;
  VkImageView image_view;
  Gpu::ImageBuffer image_buf;
};
Array<UploadedMesh, 256> buffers;
UploadedMesh *get_uploaded_mesh(String filename)
{
  for (i32 i = 0; i < buffers.size; i++) {
    if (buffers[i].filename == filename) return &buffers[i];
  }
  return nullptr;
}

Gpu::Buffer upload_mesh(Gpu::Device *gpu, StandardMesh3d *mesh)
{
  return Gpu::create_vertex_buffer(gpu, mesh->vertices_size * sizeof(Vertex));
}

void model_viewer(Gpu::Device *gpu, Gpu::Pipeline pipeline, String filename)
{
  UploadedMesh *mesh = get_uploaded_mesh(filename);
  if (!mesh) {
    StandardMesh3d mesh_data = load_mesh(filename, &tmp_allocator);
    UploadedMesh um;
    um.filename = filename;

    um.vertex_buffer = Gpu::create_vertex_buffer(
        gpu, mesh_data.vertices_size * sizeof(Vertex));
    um.index_buffer =
        Gpu::create_index_buffer(gpu, mesh_data.indexes_size * sizeof(u32));
    Gpu::upload_buffer_staged(gpu, um.vertex_buffer, mesh_data.vertices,
                              mesh_data.vertices_size * sizeof(Vertex));
    Gpu::upload_buffer_staged(gpu, um.index_buffer, mesh_data.indexes,
                              mesh_data.indexes_size * sizeof(u32));

    Image image = read_image_file(
        "../fracas/set/models/pedestal/Pedestal_Albedo.png", &system_allocator);
    um.image_buf =
        create_image(gpu, image.width, image.height, Gpu::Format::RGBA8U);
    Gpu::upload_image(gpu, um.image_buf, image);

    um.desc_set   = Gpu::create_descriptor_set(gpu, pipeline);
    um.sampler    = create_sampler(gpu, false, false);
    um.image_view = Gpu::create_image_view(gpu, um.image_buf,
                                           VkFormat::VK_FORMAT_R8G8B8A8_SRGB);
    bind_sampler(gpu, um.desc_set, um.image_view, um.sampler, 0, 0);

    i32 new_i = buffers.push_back(um);
    mesh      = &buffers[new_i];
  }

  Gpu::bind_descriptor_set(gpu, pipeline, mesh->desc_set);

  static f32 t = 0.f;
  t += .1f;

  f32 horizontal_angle = 0.f;
  Vec3f camera_pos     = {2 * cosf(t), 2 * sinf(t), 2 * sinf(t)};

  Mat4f mvp = perspective(3.1415 / 2.f, 1, 0.001, 1000) *
              look_at(camera_pos, {0, 0, 0}, normalize(Vec3f{0, 1, 0}));

  Gpu::push_constant(gpu, pipeline, &mvp, sizeof(mvp));

  Gpu::draw_indexed(gpu, mesh->vertex_buffer, mesh->index_buffer, 0,
                    mesh->index_buffer.size / sizeof(u32));
}
}  // namespace Editor