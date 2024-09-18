#pragma once

#include "types.hpp"
#include "gpu/metal/device.hpp"
#include "gpu/metal/buffer.hpp"
#include "gpu/metal/pipeline.hpp"
#include "gpu/metal/texture.hpp"
#include "gpu/shader_args.hpp"
#include "metal_headers.hpp"

namespace Gpu {
struct ShaderArgBuffer {
    Buffer buffer;
    MTL::ArgumentEncoder *arg_encoder;
};

MTL::DataType map_data_type(ShaderArgumentDefinition::Type in)
{
    if (in == ShaderArgumentDefinition::Type::DATA) {
        return MTL::DataType::DataTypePointer;
    }
    if (in == ShaderArgumentDefinition::Type::SAMPLER) {
        return MTL::DataType::DataTypeSampler;
    }

    return MTL::DataType::DataTypePointer;
}

MTL::ArgumentAccess map_access(Access in)
{
    if (in & Access::READ & Access::WRITE) {
        return MTL::ArgumentAccess::ArgumentAccessReadWrite;
    }
    if (in & Access::READ) {
        return MTL::ArgumentAccess::ArgumentAccessReadOnly;
    }
    if (in & Access::WRITE) {
        return MTL::ArgumentAccess::ArgumentAccessWriteOnly;
    }

    return MTL::ArgumentAccess::ArgumentAccessReadOnly;
}

ShaderArgBuffer create_shader_arg_buffer(Device *device, Pipeline *pipeline)
{
    Array<MTL::ArgumentDescriptor *, 16> arg_descriptors;
    for (i32 i = 0; i < pipeline->arg_defs.size; i++) {
        MTL::ArgumentDescriptor *descriptor = MTL::ArgumentDescriptor::alloc()->init();
        descriptor->setIndex(i);
        descriptor->setDataType(map_data_type(pipeline->arg_defs[i].type));
        descriptor->setAccess(map_access(pipeline->arg_defs[i].access));
        descriptor->setArrayLength(pipeline->arg_defs[i].count);
        arg_descriptors.push_back(descriptor);
    }
    NS::Array *d = NS::Array::alloc()->init((const NS::Object *const *) arg_descriptors.data, arg_descriptors.size);

    MTL::ArgumentEncoder *arg_encoder = device->metal_device->newArgumentEncoder(d);

    for (i32 i = 0; i < pipeline->arg_defs.size; i++) {
        arg_descriptors[i]->release();
    }
    d->release();

    ShaderArgBuffer arg_buffer;
    arg_buffer.arg_encoder = arg_encoder;
    arg_buffer.buffer = create_buffer(device, arg_encoder->encodedLength(), "arg_buffer");
    return arg_buffer;
}

void destroy_shader_arg_buffer(ShaderArgBuffer arg_buffer) {
    arg_buffer.arg_encoder->release();
    destroy_buffer(arg_buffer.buffer);
}

void bind_shader_buffer_data(ShaderArgBuffer arg_buffer, Buffer data_buffer, i32 index)
{
    arg_buffer.arg_encoder->setArgumentBuffer(arg_buffer.buffer.mtl_buffer, 0);
    arg_buffer.arg_encoder->setBuffer(data_buffer.mtl_buffer, 0, index);
}

void bind_shader_buffer_texture(ShaderArgBuffer arg_buffer, Texture texture, i32 index)
{
    arg_buffer.arg_encoder->setArgumentBuffer(arg_buffer.buffer.mtl_buffer, 0);
    arg_buffer.arg_encoder->setTexture(texture.mtl_texture, index);
}

}