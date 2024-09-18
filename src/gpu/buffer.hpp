#pragma once

#include "gpu/device.hpp"
#include "types.hpp"

namespace Gpu {

struct Buffer;

Buffer create_buffer(Device *device, i32 size, String name);
void destroy_buffer(Buffer buffer);
void upload_buffer(Buffer buffer, void *data, i32 size, i32 offset);

}