#pragma once

#include <GLFW/glfw3.h> 

#include "platform.hpp"

namespace Gpu {

struct Device;
Device *init(Platform::GlfwWindow *glfwWindow);

void start_frame(Device *device);
void end_frame(Device *device);

}