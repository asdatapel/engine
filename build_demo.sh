#!/bin/bash

../../../VulkanSdk/1.3.231.1/Bin/glslc.exe  -fshader-stage=fragment shaders/basic/frag.glsl -o shaders/basic/frag.spv
../../../VulkanSdk/1.3.231.1/Bin/glslc.exe  -fshader-stage=vertex shaders/basic/vert.glsl -o shaders/basic/vert.spv

export PATH=$PATH:"/mnt/c/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/Llvm/x64/bin/"
# ls "/mnt/c/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/Llvm/x64/bin/clang.exe"
clang.exe -g -std=c++17 demo.cpp -o ./build/demo.exe -I "./" -I ./thirdparty/glfw/include ./thirdparty/glfw/lib-clang/glfw3.lib -I "C:\Users\Asad\VulkanSdk\1.3.231.1\Include" "C:\Users\Asad\VulkanSdk\1.3.231.1\Lib\vulkan-1.lib"