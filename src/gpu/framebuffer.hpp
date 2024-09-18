#pragma once

#include "gpu/device.hpp"
#include "gpu/texture.hpp"

namespace Gpu
{
struct Framebuffer {
};

Framebuffer create_framebuffer(Device *device, Vec2u size)
{
  return {};
}

void start_framebuffer(Device *device, Framebuffer framebuffer)
{
}

void end_framebuffer(Device *device)
{
}
}  // namespace Gpu