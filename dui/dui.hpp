#pragma once

#ifndef DUI_HOT_RELOAD

#include "dui/dui.cpp"

#else

#define NOMINMAX
#include <windows.h>

#include "dui/api.hpp"

namespace Dui
{
// decltype (init_dui) (*init_dui_func)();
// typedef void (*debug_ui_test_func)(Input *input, Vec2f canvas_size);
// typedef Dui::DuiState *(*get_dui_state_func)();

decltype(api_init_dui)* init_dui;
decltype(api_debug_ui_test)* debug_ui_test;
decltype(api_get_dui_state)* get_dui_state;

}  // namespace Dui

void init_dui_dll()
{
  static HMODULE lib_handle = 0;

  if (lib_handle) {
    FreeLibrary(lib_handle);
  }

  DeleteFile(".\\build\\dui1.dll");
  CopyFile(".\\build\\dui.dll", ".\\build\\dui1.dll", false);

  lib_handle = LoadLibraryA(".\\build\\dui1.dll");

  Dui::init_dui = reinterpret_cast<decltype(Dui::api_init_dui)*>(
      GetProcAddress(lib_handle, "api_init_dui"));
  Dui::debug_ui_test = reinterpret_cast<decltype(Dui::api_debug_ui_test)*>(
      GetProcAddress(lib_handle, "api_debug_ui_test"));
  Dui::get_dui_state = reinterpret_cast<decltype(Dui::api_get_dui_state)*>(
      GetProcAddress(lib_handle, "api_get_dui_state"));
}

#endif