#!/bin/bash

../../../VulkanSdk/1.3.231.1/Bin/glslc.exe  -fshader-stage=fragment resources/shaders/basic/frag.glsl -o resources/shaders/basic/frag.spv
../../../VulkanSdk/1.3.231.1/Bin/glslc.exe  -fshader-stage=vertex resources/shaders/basic/vert.glsl -o resources/shaders/basic/vert.spv

../../../VulkanSdk/1.3.231.1/Bin/glslc.exe  -fshader-stage=fragment resources/shaders/dui/frag.glsl -o resources/shaders/dui/frag.spv
../../../VulkanSdk/1.3.231.1/Bin/glslc.exe  -fshader-stage=vertex resources/shaders/dui/vert.glsl -o resources/shaders/dui/vert.spv

../../../VulkanSdk/1.3.231.1/Bin/glslc.exe  -fshader-stage=fragment resources/shaders/basic_3d/frag.glsl -o resources/shaders/basic_3d/frag.spv
../../../VulkanSdk/1.3.231.1/Bin/glslc.exe  -fshader-stage=vertex resources/shaders/basic_3d/vert.glsl -o resources/shaders/basic_3d/vert.spv

export PATH=$PATH:"/mnt/c/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/Llvm/x64/bin/"
# ls "/mnt/c/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/Llvm/x64/bin/clang.exe"

# ./dui/build.sh
clang.exe -g -std=c++17 src/editor_main.cpp -o \
  ./build/editor.exe -D DUI_HOT_RELOAD \
  -I "./src/" -I "./" -I "./third_party/" -I "./third_party/glm/" \
  -I ./third_party/glfw3.4/include ./third_party/glfw3.4/glfw3dll.lib \
  -I "C:\Users\Asad\VulkanSdk\1.3.231.1\Include" "C:\Users\Asad\VulkanSdk\1.3.231.1\Lib\vulkan-1.lib"  \
  -I ./third_party/freetype/include ./third_party/freetype/debug/freetyped.lib \
  -I ./third_party/assimp/include   ./third_party/assimp/lib/assimp-vc141-mt.lib