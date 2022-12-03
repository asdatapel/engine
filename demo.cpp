#include <iostream>

#include "dui/api.hpp"
#include "platform.hpp"
#include "types.hpp"
#include "gpu/vulkan/vulkan.hpp"

struct Vert2D {
  f32 x, y;
};
struct Vert3D {
  f32 x, y, z;
};
struct Vertex {
  Vert2D pos;
  Vert3D color;
};

Vertex tri[] = {
    {{0.0, -0.5}, {1.0, 0.0, 0.0f}},
    {{0.5, 0.5}, {0.0, 1.0, 0.0f}},
    {{-0.5, 0.5}, {0.0, 0.0, 1.f}},
};

Vertex tri2[] = {
    {{0.25, -0.5}, {.9, .7, .3f}},
    {{0.75, 0.5}, {0.0, 1.0, 0.0f}},
    {{-0.25, 0.5}, {0.0, 0.0, 1.f}},
};

int main()
{
  Platform::init();

  Platform::GlfwWindow window;
  window.init();

  init_vulkan(&window);

  Pipeline pipeline = create_pipeline(vk.render_pass);

  Buffer vb = create_vertex_buffer(sizeof(tri));
  upload_buffer_staged(vb, &tri, sizeof(tri));
  Buffer vb2 = create_vertex_buffer(sizeof(tri2));
  upload_buffer_staged(vb2, &tri2, sizeof(tri2));

  Image green_tex(256, 256, 4, &system_allocator);
  for (i32 i = 0; i < 256 * 256; i++) {
    green_tex.data()[i * 4 + 1] = 255;
  }

  ImageBuffer image = create_image(256, 256);
  VkImageView image_view = create_image_view(image);
  VkSampler sampler = create_sampler();

  upload_image(image, green_tex);
  VkDescriptorSet desc_set = create_descriptor_set(pipeline);
  write_sampler(desc_set, image_view, sampler);

  Input input;
  Platform::setup_input_callbacks(&window, &input);

  while (!window.should_close()) {
    Platform::fill_input(&window, &input);

    start_frame(pipeline);

    // draw_vertex_buffer(vb, 0, 3);
    // draw_vertex_buffer(vb2, 0, 3);
    Dui::debug_ui_test(&input, pipeline, window.get_size());

    end_frame();
  }

  destroy_pipeline(pipeline);

  destroy_vulkan();
  window.destroy();

  return 0;
}