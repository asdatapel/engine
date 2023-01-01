#!/bin/bash

../../../VulkanSdk/1.3.231.1/Bin/glslc.exe  -fshader-stage=fragment shaders/basic/frag.glsl -o shaders/basic/frag.spv
../../../VulkanSdk/1.3.231.1/Bin/glslc.exe  -fshader-stage=vertex shaders/basic/vert.glsl -o shaders/basic/vert.spv

../../../VulkanSdk/1.3.231.1/Bin/glslc.exe  -fshader-stage=fragment shaders/dui/frag.glsl -o shaders/dui/frag.spv
../../../VulkanSdk/1.3.231.1/Bin/glslc.exe  -fshader-stage=vertex shaders/dui/vert.glsl -o shaders/dui/vert.spv

export PATH=$PATH:"/mnt/c/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/Llvm/x64/bin/"
# ls "/mnt/c/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/Llvm/x64/bin/clang.exe"

# ./dui/build.sh
clang.exe -g -std=c++17 demo.cpp -o ./build/demo.exe -D DUI_HOT_RELOAD -I "./" ./external/glfw3.4/glfw3dll.lib -I "C:\Users\Asad\VulkanSdk\1.3.231.1\Include" "C:\Users\Asad\VulkanSdk\1.3.231.1\Lib\vulkan-1.lib"  -I ./external/freetype/include ./external/freetype/debug/freetyped.lib