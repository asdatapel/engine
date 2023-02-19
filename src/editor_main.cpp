#include <iostream>

#include "dui/dui.hpp"
#include "editor/editor.hpp"
#include "editor/model_viewer.hpp"
#include "gpu/vulkan/framebuffer.hpp"
#include "gpu/vulkan/vulkan.hpp"
#include "model_import.hpp"
#include "platform.hpp"
#include "types.hpp"

int main()
{
  Platform::init();

  Platform::GlfwWindow window;
  window.init();

  Gpu::Device device     = Gpu::init_vulkan(&window);
  Gpu::Pipeline pipeline = Gpu::create_pipeline(
      &device,
      {"resources/shaders/dui/vert.spv", "resources/shaders/dui/frag.spv"}, {},
      device.render_pass, sizeof(Vec2f));

  Gpu::Framebuffer secondary_framebuffer =
      create_framebuffer(&device, {500, 500});
  Gpu::VertexDescription vertex_description_3d;
  vertex_description_3d.attributes = {
      Gpu::Format::RGB32F,
      Gpu::Format::RGB32F,
      Gpu::Format::RG32F,
  };
  Gpu::Pipeline pipeline3d = Gpu::create_pipeline(
      &device,
      {"resources/shaders/basic_3d/vert.spv",
       "resources/shaders/basic_3d/frag.spv"},
      vertex_description_3d, secondary_framebuffer.render_pass, sizeof(Mat4f));

  Input input;
  Platform::setup_input_callbacks(&window, &input);

  init_dui_dll();
  Dui::init_dui(&device, pipeline);

  while (!window.should_close()) {
    Platform::fill_input(&window, &input);

    Gpu::start_frame(&device);

    Gpu::start_framebuffer(&device, secondary_framebuffer);
    Gpu::bind_pipeline(&device, pipeline3d, secondary_framebuffer.size);
    Editor::model_viewer(&device, pipeline3d,
                         "../fracas/set/models/pedestal.fbx");
    Gpu::end_framebuffer(&device);

    Gpu::start_backbuffer(&device);
    Gpu::bind_pipeline(
        &device, pipeline,
        {device.swap_chain_extent.width, device.swap_chain_extent.height});
    Dui::debug_ui_test(&device, pipeline, &input, &window,
                       secondary_framebuffer);
    Editor::do_frame(&device, pipeline, &window, &input);
    Gpu::end_backbuffer(&device);

    Gpu::end_frame(&device);

    if (input.key_down_events[(i32)Keys::ENTER]) {
      Dui::ReloadData reload_data = Dui::get_plugin_reload_data();

      init_dui_dll();
      Dui::reload_plugin(reload_data);
    }
  }

  destroy_pipeline(&device, pipeline);

  destroy_vulkan(&device);
  window.destroy();

  return 0;
}