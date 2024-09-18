#!/bin/bash

# ../../../VulkanSdk/1.3.231.1/Bin/glslc.exe  -fshader-stage=fragment resources/shaders/basic/frag.glsl -o resources/shaders/basic/frag.spv
# ../../../VulkanSdk/1.3.231.1/Bin/glslc.exe  -fshader-stage=vertex resources/shaders/basic/vert.glsl -o resources/shaders/basic/vert.spv

# ../../../VulkanSdk/1.3.231.1/Bin/glslc.exe  -fshader-stage=fragment resources/shaders/dui/frag.glsl -o resources/shaders/dui/frag.spv
# ../../../VulkanSdk/1.3.231.1/Bin/glslc.exe  -fshader-stage=vertex resources/shaders/dui/vert.glsl -o resources/shaders/dui/vert.spv

# ../../../VulkanSdk/1.3.231.1/Bin/glslc.exe  -fshader-stage=fragment resources/shaders/basic_3d/frag.glsl -o resources/shaders/basic_3d/frag.spv
# ../../../VulkanSdk/1.3.231.1/Bin/glslc.exe  -fshader-stage=vertex resources/shaders/basic_3d/vert.glsl -o resources/shaders/basic_3d/vert.spv

# export PATH=$PATH:"/mnt/c/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/Llvm/x64/bin/"
# ls "/mnt/c/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/Llvm/x64/bin/clang.exe"

mkdir -p build/resources/shaders/metal/dui
xcrun -sdk macosx metal resources/shaders/metal/dui/dui.metal -c -o build/resources/shaders/metal/dui/dui.air
xcrun -sdk macosx metallib build/resources/shaders/metal/dui/dui.air -o build/resources/shaders/metal/dui/dui.metallib

# ./dui/build.sh
clang++ \
  -g -std=c++17 -fno-exceptions \
  -framework Foundation -framework Metal -framework Quartzcore -framework IOKit -framework Cocoa -framework OpenGL \
  src/editor_main.cpp \
  -o ./build/editor.exe \
  -I ./src/ -I ./ -I ./third_party/ \
  -I ./third_party/glm/ \
  -I ./third_party/metal-cpp  ./src/gpu/metal/glfw_bridge.mm ./src/gpu/metal/metal_implementation.cpp \
  -I ./third_party/glfw/include ./third_party/glfw/build/src/libglfw3.a \
  -I ./third_party/freetype/include ./third_party/freetype/build/libfreetype.a \
  -I ./third_party/vk_mem_alloc/include
  
  # -I "C:\Users\Asad\VulkanSdk\1.3.231.1\Include" "C:\Users\Asad\VulkanSdk\1.3.231.1\Lib\vulkan-1.lib"  \
  # -I ./third_party/assimp/include   ./third_party/assimp/lib/assimp-vc141-mt.lib \