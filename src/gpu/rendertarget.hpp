#pragma once

#include "gpu/device.hpp"

namespace Gpu{

struct  RenderTarget;

RenderTarget create_render_target(Device *device); 
}