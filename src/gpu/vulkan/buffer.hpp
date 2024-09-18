#pragma once

#include <vulkan/vulkan.h>

#include "containers/array.hpp"
#include "gpu/vulkan/device.hpp"
#include "logging.hpp"

namespace Gpu
{
struct Buffer {
  VkBuffer ref;
  VmaAllocation vma_allocation;
  u32 size;
};
Array<Buffer, 1024> buffers;

Buffer create_buffer(Device *device, VkDeviceSize size,
                     VkBufferUsageFlags usage,
                     VmaAllocationCreateFlags vma_alloc_create_flags)
{
  Buffer buffer;
  buffer.size = size;

  VkBufferCreateInfo buffer_info{};
  buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size        = size;
  buffer_info.usage       = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo vma_alloc_info = {};
  vma_alloc_info.usage                   = VMA_MEMORY_USAGE_AUTO;
  vma_alloc_info.flags                   = vma_alloc_create_flags;

  // allocate the buffer
  if (vmaCreateBuffer(device->vma_allocator, &buffer_info, &vma_alloc_info,
                      &buffer.ref, &buffer.vma_allocation,
                      nullptr) != VK_SUCCESS) {
    fatal("failed to allocate buffer!");
  };

  // if (vkCreateBuffer(device->device, &buffer_info, nullptr, &buffer.ref) !=
  //     VK_SUCCESS) {
  //   fatal("failed to create buffer!");
  // }

  // VkMemoryRequirements mem_requirements;
  // vkGetBufferMemoryRequirements(device->device, buffer.ref,
  // &mem_requirements);

  // VkMemoryAllocateInfo alloc_info{};
  // alloc_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  // alloc_info.allocationSize = mem_requirements.size;
  // alloc_info.memoryTypeIndex =
  //     find_memory_type(device, mem_requirements.memoryTypeBits, properties);

  // if (vkAllocateMemory(device->device, &alloc_info, nullptr, &buffer.memory)
  // !=
  //     VK_SUCCESS) {
  //   fatal("failed to allocate buffer memory!");
  // }

  // vkBindBufferMemory(device->device, buffer.ref, buffer.memory, 0);

  return buffer;
}

void destroy_buffer(Device *device, Buffer b)
{
  vmaDestroyBuffer(device->vma_allocator, b.ref, b.vma_allocation);
}

void copy_buffer(Device *device, Buffer src, Buffer dst, u64 size)
{
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = device->command_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(device->device, &alloc_info, &command_buffer);

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(command_buffer, &begin_info);

  VkBufferCopy copy_region{};
  copy_region.srcOffset = 0;  // Optional
  copy_region.dstOffset = 0;  // Optional
  copy_region.size      = size;
  vkCmdCopyBuffer(command_buffer, src.ref, dst.ref, 1, &copy_region);

  vkEndCommandBuffer(command_buffer);

  VkSubmitInfo submit_info{};
  submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers    = &command_buffer;

  vkQueueSubmit(device->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(device->graphics_queue);

  vkFreeCommandBuffers(device->device, device->command_pool, 1,
                       &command_buffer);
}

void upload_buffer_staged(Device *device, Buffer buffer, void *data, u32 size)
{
  if (size == 0) return;

  Buffer staging_buffer =
      create_buffer(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                        VMA_ALLOCATION_CREATE_MAPPED_BIT);

  void *dest = staging_buffer.vma_allocation->GetMappedData();
  memcpy(dest, data, size);

  copy_buffer(device, staging_buffer, buffer, size);

  destroy_buffer(device, staging_buffer);
}

void upload_buffer(Device *device, Buffer buffer, void *data, u32 size)
{
  void *dest;
  vkMapMemory(device->device, buffer.vma_allocation->GetMemory(),
              buffer.vma_allocation->GetOffset(), size, 0, &dest);
  memcpy(dest, data, size);
  vkUnmapMemory(device->device, buffer.vma_allocation->GetMemory());
}

Buffer create_buffer(Device *device, u32 size)
{
  Buffer vertex_buffer = create_buffer(
      device, size,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | 
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 0);
  buffers.push_back(vertex_buffer);

  return vertex_buffer;
};

void draw_vertex_buffer(Device *device, Buffer b, u32 offset, u32 vert_count)
{
  VkBuffer vertex_buffers[] = {b.ref};
  VkDeviceSize offsets[]    = {0};
  vkCmdBindVertexBuffers(device->command_buffer, 0, 1, vertex_buffers, offsets);

  vkCmdDraw(device->command_buffer, vert_count, 1, offset, 0);
};

void draw_indexed(Device *device, Buffer index_buffer, u32 offset,
                  u32 vert_count)
{
  // vkCmdBindVertexBuffers(device->command_buffer, 0, 0, nullptr, nullptr);
  vkCmdBindIndexBuffer(device->command_buffer, index_buffer.ref, 0,
                       VK_INDEX_TYPE_UINT32);

  vkCmdDrawIndexed(device->command_buffer, vert_count, 1, offset, 0, 0);
};

void draw_indexed(Device *device, Buffer vertex_buffer, Buffer index_buffer,
                  u32 offset, u32 vert_count)
{
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(device->command_buffer, 0, 1, &vertex_buffer.ref,
                         offsets);
  vkCmdBindIndexBuffer(device->command_buffer, index_buffer.ref, 0,
                       VK_INDEX_TYPE_UINT32);

  vkCmdDrawIndexed(device->command_buffer, vert_count, 1, offset, 0, 0);
};

}  // namespace Gpu