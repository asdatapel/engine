#pragma once

#include "gpu/metal/pipeline.hpp"
#include "image.hpp"

namespace Gpu {

struct Texture {
    MTL::Texture* mtl_texture;
};

Texture create_texture(Device *device, Image image)
{
    Texture texture;

    MTL::TextureDescriptor* texture_descriptor = MTL::TextureDescriptor::alloc()->init();
    texture_descriptor->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
    texture_descriptor->setWidth(image.width);
    texture_descriptor->setHeight(image.height);

    texture.mtl_texture = device->metal_device->newTexture(texture_descriptor);

    MTL::Region region = MTL::Region(0, 0, 0, image.width, image.height, 1);
    NS::UInteger bytes_per_row = 4 * image.width;
    texture.mtl_texture->replaceRegion(region, 0, image.data(), bytes_per_row);

    texture_descriptor->release();

    return texture;
}

void bind_sampler(Device *device, ShaderArgs shader_args, u32 binding,
                  u32 index = 0) {

                  }

}