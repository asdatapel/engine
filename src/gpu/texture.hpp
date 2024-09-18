#pragma once

#include "gpu/device.hpp"
// #include "gpu/vulkan/vulkan.hpp"
#include "image.hpp"

namespace Gpu
{
struct Texture;

Texture create_texture(Device *device, Image image);
Texture create_texture(Device *device, u32 width, u32 height, PixelFormat format);
Texture destroy_texture(Texture texture);

void upload_texture(Device *device, Texture texture, Image image);
void upload_texture(Device *device, Texture texture, void *data, i32 size, i32 offset);

}  // namespace Gpu