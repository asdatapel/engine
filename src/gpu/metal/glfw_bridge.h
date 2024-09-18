#pragma once

struct GLFWwindow;

namespace CA
{
class MetalLayer;
}
namespace GLFWBridge
{
void AddLayerToWindow(GLFWwindow* window, CA::MetalLayer* layer);
}