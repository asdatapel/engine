#pragma once


#define NOMINMAX
#include <windows.h>
#undef near
#undef far

#include <unordered_map>

#include "dui/dui_state.hpp"
#include "gpu/vulkan/framebuffer.hpp"
#include "input.hpp"
#include "logging.hpp"
#include "math/math.hpp"
#include "plugin.hpp"

namespace Dui
{

struct ReloadData {
  DuiState *dui_state;
};

API DuiState *api_get_dui_state();
API ReloadData api_get_plugin_reload_data();
API void api_reload_plugin(ReloadData reload_data);
API void api_init_dui(Gpu::Device *device, Gpu::Pipeline pipeline);
API void api_debug_ui_test(Gpu::Device *device, Gpu::Pipeline pipeline,
                           Input *input, Platform::GlfwWindow *window,
                           Gpu::Framebuffer framebuffer);
API void api_start_frame(Input *input, Platform::GlfwWindow *window);
API void api_end_frame(Platform::GlfwWindow *window, Gpu::Device *device,
                   Gpu::Pipeline pipeline);
API DuiId api_start_window(String name, Rect initial_rect);
API void api_end_window();
API b8 api_button(String text, Vec2f size, Color color, b8 fill = false);
API void api_texture(Vec2f size, u32 texture_id);
API void api_next_line();

decltype(api_get_plugin_reload_data) *get_plugin_reload_data;
decltype(api_reload_plugin) *reload_plugin;
decltype(api_init_dui) *init_dui;
decltype(api_debug_ui_test) *debug_ui_test;
decltype(api_get_dui_state) *get_dui_state;
decltype(api_start_frame) *start_frame;
decltype(api_end_frame) *end_frame;
decltype(api_start_window) *start_window;
decltype(api_end_window) *end_window;
decltype(api_button) *button;
decltype(api_texture) *texture;
decltype(api_next_line) *next_line;

}  // namespace Dui

static HMODULE lib_handles[] = {0, 0};

void init_dui_dll()
{
  static i32 which = 0;

  if (lib_handles[which]) {
    FreeLibrary(lib_handles[which]);
  }

  i32 random = rand() % 100000;
  char filename[128];
  char command[128];
  sprintf(filename, "./build/dui/dui%i.dll", random);
  sprintf(command, "bash ./src/dui/build.sh ./build/dui/dui%i.dll", random);

  STARTUPINFO info = {sizeof(info)};
  PROCESS_INFORMATION processInfo;
  CreateProcess(NULL, command, nullptr, nullptr, true, 0, nullptr, nullptr,
                &info, &processInfo);
  WaitForSingleObject(processInfo.hProcess, 5000);
  CloseHandle(processInfo.hProcess);
  CloseHandle(processInfo.hThread);

  lib_handles[which] = LoadLibraryA(filename);

  Dui::get_plugin_reload_data =
      reinterpret_cast<decltype(Dui::api_get_plugin_reload_data) *>(
          GetProcAddress(lib_handles[which], "api_get_plugin_reload_data"));
  Dui::reload_plugin = reinterpret_cast<decltype(Dui::api_reload_plugin) *>(
      GetProcAddress(lib_handles[which], "api_reload_plugin"));
  Dui::init_dui = reinterpret_cast<decltype(Dui::api_init_dui) *>(
      GetProcAddress(lib_handles[which], "api_init_dui"));
  Dui::debug_ui_test = reinterpret_cast<decltype(Dui::api_debug_ui_test) *>(
      GetProcAddress(lib_handles[which], "api_debug_ui_test"));
  Dui::get_dui_state = reinterpret_cast<decltype(Dui::api_get_dui_state) *>(
      GetProcAddress(lib_handles[which], "api_get_dui_state"));
      
  Dui::start_frame = reinterpret_cast<decltype(Dui::api_start_frame) *>(
      GetProcAddress(lib_handles[which], "api_start_frame"));
  Dui::end_frame = reinterpret_cast<decltype(Dui::api_end_frame) *>(
      GetProcAddress(lib_handles[which], "api_end_frame"));
  Dui::start_window = reinterpret_cast<decltype(Dui::api_start_window) *>(
      GetProcAddress(lib_handles[which], "api_start_window"));
  Dui::end_window = reinterpret_cast<decltype(Dui::api_end_window) *>(
      GetProcAddress(lib_handles[which], "api_end_window"));
  Dui::button = reinterpret_cast<decltype(Dui::api_button) *>(
      GetProcAddress(lib_handles[which], "api_button"));
  Dui::texture = reinterpret_cast<decltype(Dui::api_texture) *>(
      GetProcAddress(lib_handles[which], "api_texture"));
  Dui::next_line = reinterpret_cast<decltype(Dui::api_next_line) *>(
      GetProcAddress(lib_handles[which], "api_next_line"));

  which = 1 - which;
}
#ifdef DUI_HOT_RELOAD

#else

// #include "dui/dui.cpp"

#endif