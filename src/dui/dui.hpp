#pragma once

// #define NOMINMAX
// #include <windows.h>
// #undef near
// #undef far

#include <unordered_map>

#include "dui/dui_state.hpp"
#include "gpu/gpu.hpp"
#include "input.hpp"
#include "logging.hpp"
#include "math/math.hpp"
#include "plugin.hpp"

namespace Dui
{

void init_dui(Gpu::Device *device, Gpu::Pipeline pipeline);
void start_frame(Input *input, Platform::GlfwWindow *window);
void end_frame(Platform::GlfwWindow *window, Gpu::Device *device,
               Gpu::Pipeline pipeline);
DuiId start_window(String name, Engine::Rect initial_rect);
void end_window();
b8 button(String text, Vec2f size, Color color, b8 fill = false);
void texture(Vec2f size, u32 texture_id);
void next_line();

}  // namespace Dui

#include "dui/dui.cpp"

// static HMODULE lib_handles[] = {0, 0};

// void init_dui_dll()
// {
//   static i32 which = 0;

//   if (lib_handles[which]) {
//     FreeLibrary(lib_handles[which]);
//   }

//   i32 random = rand() % 100000;
//   char filename[128];
//   char command[128];
//   sprintf(filename, "./build/dui/dui%i.dll", random);
//   sprintf(command, "bash ./src/dui/build.sh ./build/dui/dui%i.dll", random);

//   STARTUPINFO info = {sizeof(info)};
//   PROCESS_INFORMATION processInfo;
//   CreateProcess(NULL, command, nullptr, nullptr, true, 0, nullptr, nullptr,
//                 &info, &processInfo);
//   WaitForSingleObject(processInfo.hProcess, 5000);
//   CloseHandle(processInfo.hProcess);
//   CloseHandle(processInfo.hThread);

//   lib_handles[which] = LoadLibraryA(filename);

//   Dui::get_plugin_reload_data =
//       reinterpret_cast<decltype(Dui::api_get_plugin_reload_data) *>(
//           GetProcAddress(lib_handles[which], "api_get_plugin_reload_data"));
//   Dui::reload_plugin = reinterpret_cast<decltype(Dui::api_reload_plugin) *>(
//       GetProcAddress(lib_handles[which], "api_reload_plugin"));
//   Dui::init_dui = reinterpret_cast<decltype(Dui::api_init_dui) *>(
//       GetProcAddress(lib_handles[which], "api_init_dui"));
//   Dui::debug_ui_test = reinterpret_cast<decltype(Dui::api_debug_ui_test) *>(
//       GetProcAddress(lib_handles[which], "api_debug_ui_test"));
//   Dui::get_dui_state = reinterpret_cast<decltype(Dui::api_get_dui_state) *>(
//       GetProcAddress(lib_handles[which], "api_get_dui_state"));

//   Dui::start_frame = reinterpret_cast<decltype(Dui::api_start_frame) *>(
//       GetProcAddress(lib_handles[which], "api_start_frame"));
//   Dui::end_frame = reinterpret_cast<decltype(Dui::api_end_frame) *>(
//       GetProcAddress(lib_handles[which], "api_end_frame"));
//   Dui::start_window = reinterpret_cast<decltype(Dui::api_start_window) *>(
//       GetProcAddress(lib_handles[which], "api_start_window"));
//   Dui::end_window = reinterpret_cast<decltype(Dui::api_end_window) *>(
//       GetProcAddress(lib_handles[which], "api_end_window"));
//   Dui::button = reinterpret_cast<decltype(Dui::api_button) *>(
//       GetProcAddress(lib_handles[which], "api_button"));
//   Dui::texture = reinterpret_cast<decltype(Dui::api_texture) *>(
//       GetProcAddress(lib_handles[which], "api_texture"));
//   Dui::next_line = reinterpret_cast<decltype(Dui::api_next_line) *>(
//       GetProcAddress(lib_handles[which], "api_next_line"));

//   which = 1 - which;
// }