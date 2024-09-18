#include <iostream>

#include "dui/dui.hpp"
#include "editor/editor.hpp"
#include "gpu/gpu.hpp"
#include "platform.hpp"
#include "editor/material_editor.hpp"
#include "types.hpp"

int main()
{
  Platform::init();

  Platform::GlfwWindow window;
  window.init();

  Gpu::Device *device    = Gpu::init(&window);

  Input input;
  Platform::setup_input_callbacks(&window, &input);

  Dui::init_dui(device);

  while (!window.should_close()) {
    Platform::fill_input(&window, &input);

    Gpu::start_frame(device);
    
    // Gpu::start_framebuffer(device, secondary_framebuffer);
    // Gpu::end_framebuffer(device);

    // Gpu::start_backbuffer(device);
    Editor::do_frame(device, &window, &input);
    // Gpu::end_backbuffer(device);

    Gpu::end_frame(device);
    
    tmp_allocator.reset();
  }

  // Dui::destroy()
  // Gpu::destroy_device()

  window.destroy();

  return 0;
}