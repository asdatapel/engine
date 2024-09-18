#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "memory.hpp"
#include "math/math.hpp"
#include "types.hpp"
#include "file.hpp"

struct Image {
  u32 width, height;
  u64 size;
  Mem mem;
  PixelFormat format;

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

Image read_image_file(String path, Allocator *allocator)
{
  Temp temp;
  File file = read_file(path, &temp);

  Vec2i size;
  i32 channels       = 0;
  stbi_set_flip_vertically_on_load(true);
  stbi_uc *stb_image = stbi_load_from_memory(file.data.data, file.data.size,
                                             &size.x, &size.y, &channels, 4);

  Image image(size.x, size.y, 4, allocator);
  memcpy(image.data(), stb_image, size.x * size.y * 4);

  stbi_image_free(stb_image);

  return image;
}