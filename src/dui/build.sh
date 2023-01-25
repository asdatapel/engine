#!/bin/bash
export PATH=$PATH:"/mnt/c/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/Llvm/x64/bin/"
export PATH=$PATH:"/mnt/c/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/bin/Hostx64/x64"

clang.exe -g -c -std=c++17 -o ./build/dui.o ./src/dui/plugin.cpp -I "./src/" -I "./" -I "C:\Users\Asad\VulkanSdk\1.3.231.1\Include" -I ./external/freetype/include
clang.exe -g -shared -o $1 ./build/dui.o ./external/glfw3.4/glfw3dll.lib "C:\Users\Asad\VulkanSdk\1.3.231.1\Lib\vulkan-1.lib" ./external/freetype/debug/freetyped.lib
