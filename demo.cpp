#include <iostream>

#include "dui/draw.hpp"
#include "dui/dui.hpp"
#include "dui/dui_state.hpp"
#include "gpu/vulkan/vulkan.hpp"
#include "platform.hpp"
#include "types.hpp"

int main()
{
  Platform::init();

  Platform::GlfwWindow window;
  window.init();

  Device device = init_vulkan(&window);

  Pipeline pipeline = create_pipeline(&device, device.render_pass);

  Input input;
  Platform::setup_input_callbacks(&window, &input);

  init_dui_dll();
  Dui::init_dui(&device, pipeline);

  while (!window.should_close()) {
    Platform::fill_input(&window, &input);

    start_frame(&device, pipeline);

    Dui::debug_ui_test(&device, pipeline, &input, &window, window.get_size());

    end_frame(&device);

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