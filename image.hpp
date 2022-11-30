#pragma once

#include "memory.hpp"
#include "types.hpp"

struct Image {
  u32 width, height;
  u64 size;
  Mem mem;

  Image() {}
  Image(u32 width, u32 height, u32 pixel_size, Allocator *allocator)
  {
    this->width  = width;
    this->height = height;
    this->size   = width * height * pixel_size;

    mem = allocator->alloc(size);
  }

  u8 *data() { return mem.data; };
};