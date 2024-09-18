#pragma once

#include <GLFW/glfw3.h> 

#include "gpu/gpu.hpp"
#include "platform.hpp"
#include "gpu/metal/glfw_bridge.h"
#include "gpu/metal/metal_headers.hpp"

namespace Gpu {

struct Vertex {
    Vec3f position;
    Vec2f uv;
};

struct Mesh {
    i32 vert_count;
    i32 vert_size;
    i32 tri_count;

    Vertex *vertices;
    u16   *indices;
};

Mesh create_mesh() {
    Mesh mesh;
    mesh.vert_count = 4;
    mesh.vert_size = sizeof(Vertex);
    mesh.tri_count = 2;
    mesh.vertices = (Vertex *) malloc(mesh.vert_count * sizeof(Vertex));
    mesh.indices  = (u16 *)   malloc(mesh.tri_count * 3 * sizeof(u16));

    mesh.vertices[0] = {{-0.5f,  0.5f, 0.0f}, {0.0f, 1.0f}};
    mesh.vertices[1] = {{ 0.5f,  0.5f, 0.0f}, {1.0f, 1.0f}};
    mesh.vertices[2] = {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}};
    mesh.vertices[3] = {{ 0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}};

    u16 indices[6] = {0, 1, 2, 1, 3, 2};
    memcpy(mesh.indices, indices, mesh.tri_count * 3 * sizeof(u16));

    return mesh;
}

struct Device {
    NS::AutoreleasePool *auto_release_pool = NS::AutoreleasePool::alloc()->init();
    CA::MetalLayer *metal_layer;

    MTL::Device *metal_device;
    MTL::Library *shader_library;
    MTL::CommandQueue *metal_command_queue;

    // per frame
    CA::MetalDrawable *metal_drawable;
    MTL::CommandBuffer *metal_command_buffer;
    MTL::RenderCommandEncoder *render_command_encoder;

    // temp
    MTL::RenderPipelineState *metal_render_pso;
    MTL::Buffer* vert_buffer;
    MTL::Buffer* index_buffer;
};

Device *init(Platform::GlfwWindow *glfwWindow) {
    Device *device = new Device();
    device->auto_release_pool = NS::AutoreleasePool::alloc()->init();
    
    device->metal_device = MTL::CreateSystemDefaultDevice();

    device->metal_layer = CA::MetalLayer::layer();
    device->metal_layer->setDevice(device->metal_device);
    device->metal_layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    GLFWBridge::AddLayerToWindow(glfwWindow->ref, device->metal_layer);

    NS::String* mystring = NS::String::string("build/resources/shaders/metal/dui/dui.metallib", NS::StringEncoding::UTF8StringEncoding);
    NS::Error* err = NS::Error::alloc();
    device->shader_library = device->metal_device->newLibrary(mystring, &err);
    if(!device->shader_library){
        std::cerr << "Failed to load metal shader library: " << err->debugDescription();
        std::exit(-1);
    }
    err->release();

    {
        // Mesh mesh = create_mesh();
        // device->vert_buffer = 
        //     device->metal_device->newBuffer(mesh.vertices,
        //     mesh.vert_count * mesh.vert_size,
        //     MTL::ResourceStorageModeShared);
        // device->index_buffer = 
        //     device->metal_device->newBuffer(mesh.indices,
        //         mesh.tri_count * 3 * sizeof(u16),
        //         MTL::ResourceStorageModeShared);

        // NS::String* mystring = NS::String::string("build/resources/shaders/metal/dui/dui.metallib", NS::StringEncoding::UTF8StringEncoding);
        // NS::Error* err = NS::Error::alloc();
        // MTL::Library* library = device->metal_device->newLibrary(mystring, &err);
        // if(!library){
        //     std::cerr << "Failed to load asdas library.\n" << err->debugDescription()->cString(NS::ASCIIStringEncoding) << '\n';
        //     std::exit(-1);
        // }
        // err->release();

        // command queue
        device->metal_command_queue = device->metal_device->newCommandQueue();

        // render pipeline
        // MTL::Function* vertexShader = device->shader_library->newFunction(NS::String::string("vertex_shader", NS::ASCIIStringEncoding));
        // assert(vertexShader);
        // MTL::Function* fragmentShader = device->shader_library->newFunction(NS::String::string("fragment_shader", NS::ASCIIStringEncoding));
        // assert(fragmentShader);

        // MTL::RenderPipelineDescriptor* renderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
        // renderPipelineDescriptor->setLabel(NS::String::string("Triangle Rendering Pipeline", NS::ASCIIStringEncoding));
        // renderPipelineDescriptor->setVertexFunction(vertexShader);
        // renderPipelineDescriptor->setFragmentFunction(fragmentShader);
        // assert(renderPipelineDescriptor);
        // MTL::PixelFormat pixelFormat = (MTL::PixelFormat)device->metal_layer->pixelFormat();
        // renderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(pixelFormat);

        // NS::Error* error;
        // device->metal_render_pso = device->metal_device->newRenderPipelineState(renderPipelineDescriptor, &error);

        // renderPipelineDescriptor->release();
    }

    return device;
}

void start_frame(Device *device) {  
    device->metal_drawable = device->metal_layer->nextDrawable();

    MTL::RenderPassDescriptor* render_pass_descriptor = MTL::RenderPassDescriptor::alloc()->init();
    MTL::RenderPassColorAttachmentDescriptor* cd = render_pass_descriptor->colorAttachments()->object(0);
    cd->setTexture(device->metal_drawable->texture());
    cd->setLoadAction(MTL::LoadActionClear);
    cd->setClearColor(MTL::ClearColor(41.0f/255.0f, 42.0f/255.0f, 48.0f/255.0f, 1.0));
    cd->setStoreAction(MTL::StoreActionStore);

    device->metal_command_buffer = device->metal_command_queue->commandBuffer();
    device->render_command_encoder = device->metal_command_buffer->renderCommandEncoder(render_pass_descriptor);
    
    render_pass_descriptor->release();
}

void end_frame(Device *device) {device->render_command_encoder->endEncoding();
    device->metal_command_buffer->presentDrawable(device->metal_drawable);
    device->metal_command_buffer->commit();
    device->metal_command_buffer->waitUntilCompleted();

    device->auto_release_pool->release();
    device->auto_release_pool = NS::AutoreleasePool::alloc()->init();
}

}