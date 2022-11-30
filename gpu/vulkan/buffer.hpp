#pragma once

#include <vulkan/vulkan.h>

#include "containers/array.hpp"
#include "gpu/vulkan/device.hpp"
#include "logging.hpp"

struct Buffer {
  VkBuffer ref;
  VkDeviceMemory memory;
  u32 size;
};
Array<Buffer, 1024> buffers;

Buffer create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties)
{
  Buffer buffer;
  buffer.size = size;

  VkBufferCreateInfo buffer_info{};
  buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size        = size;
  buffer_info.usage       = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(vk.device, &buffer_info, nullptr, &buffer.ref) !=
      VK_SUCCESS) {
    fatal("failed to create buffer!");
  }

  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(vk.device, buffer.ref, &mem_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex =
      find_memory_type(mem_requirements.memoryTypeBits, properties);

  if (vkAllocateMemory(vk.device, &alloc_info, nullptr, &buffer.memory) !=
      VK_SUCCESS) {
    fatal("failed to allocate buffer memory!");
  }

  vkBindBufferMemory(vk.device, buffer.ref, buffer.memory, 0);

  return buffer;
}

void destroy_buffer(Buffer b)
{
  vkDestroyBuffer(vk.device, b.ref, nullptr);
  vkFreeMemory(vk.device, b.memory, nullptr);
}

void copy_buffer(Buffer src, Buffer dst, u64 size)
{
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = vk.command_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(vk.device, &alloc_info, &command_buffer);

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

  vkQueueSubmit(vk.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(vk.graphics_queue);

  vkFreeCommandBuffers(vk.device, vk.command_pool, 1, &command_buffer);
}

void upload_buffer_staged(Buffer buffer, void *data, u32 size)
{
  if (size == 0) return;

  Buffer staging_buffer =
      create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  void *dest;
  vkMapMemory(vk.device, staging_buffer.memory, 0, size, 0, &dest);
  memcpy(dest, data, size);
  vkUnmapMemory(vk.device, staging_buffer.memory);

  copy_buffer(staging_buffer, buffer, size);

  destroy_buffer(staging_buffer);
}

void upload_buffer(Buffer buffer, void *data, u32 size)
{
  void *dest;
  vkMapMemory(vk.device, buffer.memory, 0, size, 0, &dest);
  memcpy(dest, data, size);
  vkUnmapMemory(vk.device, buffer.memory);
}

Buffer create_vertex_buffer(u32 size)
{
  Buffer vertex_buffer = create_buffer(
      size,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  buffers.push_back(vertex_buffer);

  return vertex_buffer;
};

void draw_vertex_buffer(Buffer b, u32 offset, u32 vert_count)
{
  VkBuffer vertex_buffers[] = {b.ref};
  VkDeviceSize offsets[]    = {0};
  vkCmdBindVertexBuffers(vk.command_buffer, 0, 1, vertex_buffers, offsets);

  vkCmdDraw(vk.command_buffer, vert_count, 1, offset, 0);
};