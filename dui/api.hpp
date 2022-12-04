#pragma once

#include <unordered_map>

#include "dui/dui_state.hpp"
#include "input.hpp"
#include "math/math.hpp"
#include "plugin.hpp"

namespace Dui
{

API void api_init_dui(Device *device, Pipeline pipeline);
API void api_debug_ui_test(Device *device, Pipeline pipeline, Input *input,
                       Vec2f canvas_size);
API DuiState *api_get_dui_state();

}  // namespace Dui