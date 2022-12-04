#define NOMINMAX
#include <windows.h>

#include "dui/dui_state.hpp"
#include "gpu/vulkan/pipeline.hpp"
#include "input.hpp"

typedef void (*init_dui_func)();
typedef void (*debug_ui_test_func)(Input *input, Vec2f canvas_size);
typedef Dui::DuiState *(*get_dui_state_func)();

init_dui_func init_dui;
debug_ui_test_func debug_ui_test;
get_dui_state_func get_dui_state;

#define DUI_HOT_RELOAD

#ifdef DUI_HOT_RELOAD
void init_dui_dll()
{
  static HMODULE lib_handle = 0;

  if (lib_handle) {
    FreeLibrary(lib_handle);
  }

  DeleteFile(".\\build\\dui1.dll");
  MoveFile(".\\build\\dui.dll", ".\\build\\dui1.dll");

  lib_handle = LoadLibraryA(".\\build\\dui1.dll");

  init_dui = (init_dui_func)GetProcAddress(lib_handle, "init_dui");
  debug_ui_test =
      (debug_ui_test_func)GetProcAddress(lib_handle, "debug_ui_test");
  get_dui_state =
      (get_dui_state_func)GetProcAddress(lib_handle, "get_dui_state");
}
#endif