#pragma once

#ifdef DUI_HOT_RELOAD

#define NOMINMAX
#include <windows.h>

#include "dui/api.hpp"
#include "logging.hpp"

namespace Dui
{
// decltype (init_dui) (*init_dui_func)();
// typedef void (*debug_ui_test_func)(Input *input, Vec2f canvas_size);
// typedef Dui::DuiState *(*get_dui_state_func)();

decltype(api_get_plugin_reload_data)* get_plugin_reload_data;
decltype(api_reload_plugin)* reload_plugin;
decltype(api_init_dui)* init_dui;
decltype(api_debug_ui_test)* debug_ui_test;
decltype(api_get_dui_state)* get_dui_state;

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
  sprintf(command, "bash dui/build.sh ./build/dui/dui%i.dll", random);

  STARTUPINFO info = {sizeof(info)};
  PROCESS_INFORMATION processInfo;
  CreateProcess(NULL, command, nullptr, nullptr, true, 0, nullptr, nullptr,
                &info, &processInfo);
  WaitForSingleObject(processInfo.hProcess, 5000);
  CloseHandle(processInfo.hProcess);
  CloseHandle(processInfo.hThread);

  lib_handles[which] = LoadLibraryA(filename);

  Dui::get_plugin_reload_data =
      reinterpret_cast<decltype(Dui::api_get_plugin_reload_data)*>(
          GetProcAddress(lib_handles[which], "api_get_plugin_reload_data"));
  Dui::reload_plugin = reinterpret_cast<decltype(Dui::api_reload_plugin)*>(
      GetProcAddress(lib_handles[which], "api_reload_plugin"));
  Dui::init_dui = reinterpret_cast<decltype(Dui::api_init_dui)*>(
      GetProcAddress(lib_handles[which], "api_init_dui"));
  Dui::debug_ui_test = reinterpret_cast<decltype(Dui::api_debug_ui_test)*>(
      GetProcAddress(lib_handles[which], "api_debug_ui_test"));
  Dui::get_dui_state = reinterpret_cast<decltype(Dui::api_get_dui_state)*>(
      GetProcAddress(lib_handles[which], "api_get_dui_state"));

  which = 1 - which;
}

#else

#include "dui/dui.cpp"

#endif