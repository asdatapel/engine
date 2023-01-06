#pragma once

#include <unordered_map>

#include "dui/dui_state.hpp"
#include "input.hpp"
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
API void api_init_dui(Device *device, Pipeline pipeline);
API void api_debug_ui_test(Device *device, Pipeline pipeline, Input *input,
                           Platform::GlfwWindow *window);

}  // namespace Dui