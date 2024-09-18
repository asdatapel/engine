#pragma once

#include "gpu/buffer.hpp"
#include "gpu/device.hpp"
#include "types.hpp"

namespace Gpu {

void draw_indexed(Device *device, Buffer index_buffer, i32 offset, i32 index_count);

}