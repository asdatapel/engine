#include <iostream>

#include "dui/dui.hpp"
#include "platform.hpp"
#include "types.hpp"
#include "vulkan.hpp"

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

  Vulkan vulkan;
  vulkan.init(&window);

  Vulkan::Buffer vb = vulkan.create_vertex_buffer(sizeof(tri));
  vulkan.upload_buffer(vb, &tri, sizeof(tri));
  Vulkan::Buffer vb2 = vulkan.create_vertex_buffer(sizeof(tri2));
  vulkan.upload_buffer(vb2, &tri2, sizeof(tri2));

  Input input;
  Platform::setup_input_callbacks(&window, &input);

  while (!window.should_close()) {
    Platform::fill_input(&window, &input);

    vulkan.start_frame();

    Dui::debug_ui_test(&input, &vulkan, window.get_size());
    vulkan.draw_vertex_buffer(vb, 0, 3);
    vulkan.draw_vertex_buffer(vb2, 0, 3);
  
    vulkan.end_frame();
  }

  vulkan.destroy();
  window.destroy();

  return 0;
}