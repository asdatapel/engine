#pragma once

#include "dui/draw.hpp"
#include "dui/dui.hpp"
#include "dui/dui_state.hpp"
#include "editor/asset_browser.hpp"
#include "editor/state.hpp"
#include "editor/material_editor.hpp"
#include "gpu/vulkan/pipeline.hpp"

namespace Editor
{

Editor::State state;

Color l_dark = Color::from_int(0x003e71);  //{1, 0.31373, 0.01176, 1};

State init() {return {};}

void do_frame(Gpu::Device *gpu, Gpu::Pipeline pipeline,
              Platform::GlfwWindow *window, Input *input)
{
  Dui::start_frame(input, window);

  DuiId w1 =
      Dui::start_window("Properties: tm_checkboard_tab", {100, 200, 300, 300});
  Dui::end_window();

  DuiId w2 =
      Dui::start_window("A really long window title", {200, 300, 100, 300});
  Dui::end_window();

  DuiId w3 = Dui::start_window("third", {300, 400, 200, 300});
  Dui::end_window();

  DuiId w4 = Dui::start_window("fourth", {400, 500, 200, 300});
  Dui::end_window();

  DuiId w5 = Dui::start_window("fifth", {500, 600, 200, 300});
  if (Dui::button("test21", {200, 300}, {1, 1, 1, 0}, false)) info("test1");
  if (Dui::button("test2", {150, 100}, {1, 0, 0, 1}, false)) info("test2");
  if (Dui::button("test3", {100, 400}, {1, 1, 0, 1}, true)) info("test3");
  Dui::next_line();
  if (Dui::button("test4", {200, 600}, {0, 1, 1, 1}, false)) info("test4");
  Dui::next_line();
  if (Dui::button("test5", {200, 700}, {1, 0, 1, 1}, false)) info("test5");
  if (Dui::button("test6", {200, 300}, {1, 1, 0, 1}, false)) info("test6");
  Dui::next_line();
  if (Dui::button("test7", {200, 100}, {0, 0, 1, 1}, false)) info("test7");
  Dui::end_window();

  DuiId w6 = Dui::start_window("sizth", {600, 700, 200, 300});

  static StaticString<68> test_text_input = "Tesssting...";
  // Dui::text_input(&test_text_input);

  static StaticString<128> test_text_input2 = "Asdfing!";
  // Dui::text_input(&test_text_input2);

  if (Dui::button("test8", {200, 30}, l_dark, true)) info("test8");
  if (Dui::button("test9", {200, 30}, l_dark, true)) info("test9");
  Dui::end_window();

  DuiId w7 = Dui::start_window("aseventh", {700, 800, 200, 300});
  Dui::end_window();

  DuiId w8 = Dui::start_window("_", {100, 100, 800, 800});
  static VkSampler sampler;
  static u32 texture_id = -1;
  // static b8 tex_init = false;
  // f32 size           = 512;
  // if (!tex_init) {
  //   tex_init = true;
  //   sampler  = create_sampler(gpu, false, false);
  //   texture_id =
  //       push_texture(gpu, &s.dl, framebuffer.color_image_view, sampler);
  // }
  // Dui::texture({size, size}, texture_id);
  Dui::end_window();
  
  static AssetBrowser ab{""};
  do_asset_browser(&state, &ab);

  for (i32 i = 0; i < state.material_editor_windows.size; i++) {
    do_material_editor_window(&state, &state.material_editor_windows[i]);
  }

  Dui::end_frame(window, gpu, pipeline);
}

};  // namespace Editor