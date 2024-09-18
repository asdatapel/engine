#pragma once

#include "buffer.hpp"
#include "gpu/vulkan/device.hpp"
// #include "gpu/vulkan/vulkan.hpp"
#include "image.hpp"

namespace Gpu
{
struct ImageBuffer {
  VkImage image;
  VkDeviceMemory memory;
};

ImageBuffer create_image(Device *device, u32 width, u32 height, Format format)
{
  VkImageCreateInfo image_info{};
  image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType     = VK_IMAGE_TYPE_2D;
  image_info.extent.width  = width;
  image_info.extent.height = height;
  image_info.extent.depth  = 1;
  image_info.mipLevels     = 1;
  image_info.arrayLayers   = 1;
  image_info.samples       = VK_SAMPLE_COUNT_1_BIT;
  image_info.flags         = 0;  // Optional
  image_info.format        = to_vk_format(format);
  image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_info.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkImage image_ref;
  if (vkCreateImage(device->device, &image_info, nullptr, &image_ref) !=
      VK_SUCCESS) {
    fatal("failed to create image!");
  }

  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(device->device, image_ref, &mem_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex =
      find_memory_type(device, mem_requirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  VkDeviceMemory memory;
  if (vkAllocateMemory(device->device, &alloc_info, nullptr, &memory) !=
      VK_SUCCESS) {
    fatal("failed to allocate memory!");
  }

  vkBindImageMemory(device->device, image_ref, memory, 0);

  ImageBuffer image;
  image.image  = image_ref;
  image.memory = memory;
  return image;
};

void transition_image_layout(Device *device, VkCommandBuffer command_buffer,
                             ImageBuffer image_buf, VkImageLayout old_layout,
                             VkImageLayout new_layout)
{
  {
    VkImageMemoryBarrier barrier{};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout           = old_layout;
    barrier.newLayout           = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image                           = image_buf.image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    barrier.srcAccessMask = 0;  // TODO
    barrier.dstAccessMask = 0;  // TODO

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;
    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
      barrier.srcAccessMask = 0;
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

      source_stage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      source_stage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
      destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
      fatal("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);
  }
}

void upload_image(Device *device, ImageBuffer image_buf, Image image)
{
  if (image.size == 0) return;

  Buffer staging_buffer =
      create_buffer(device, image.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

  upload_buffer(device, staging_buffer, image.data(), image.size);

  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = device->command_pool;
  alloc_info.commandBufferCount = 1;

  VkCommandBuffer temp_command_buffer;
  vkAllocateCommandBuffers(device->device, &alloc_info, &temp_command_buffer);

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(temp_command_buffer, &begin_info);

  transition_image_layout(device, temp_command_buffer, image_buf,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  {
    VkBufferImageCopy region{};
    region.bufferOffset      = 0;
    region.bufferRowLength   = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {image.width, image.height, 1};

    vkCmdCopyBufferToImage(temp_command_buffer, staging_buffer.ref,
                           image_buf.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  }
  transition_image_layout(device, temp_command_buffer, image_buf,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkEndCommandBuffer(temp_command_buffer);

  VkSubmitInfo submit_info{};
  submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers    = &temp_command_buffer;

  vkQueueSubmit(device->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(device->graphics_queue);

  vkFreeCommandBuffers(device->device, device->command_pool, 1,
                       &temp_command_buffer);

  destroy_buffer(device, staging_buffer);
}

VkImageView create_image_view(Device *device, ImageBuffer image_buf,
                              VkFormat format)
{
  VkImageViewCreateInfo view_info{};
  view_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image    = image_buf.image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format   = format;
  view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.baseMipLevel   = 0;
  view_info.subresourceRange.levelCount     = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount     = 1;

  VkImageView image_view;
  if (vkCreateImageView(device->device, &view_info, nullptr, &image_view) !=
      VK_SUCCESS) {
    fatal("failed to create texture image view!");
  }

  return image_view;
}

VkSampler create_sampler(Device *device, b8 linear_filter = true,
                         b8 repeat = true)
{
  VkSamplerCreateInfo sampler_info{};
  sampler_info.sType     = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter = linear_filter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
  sampler_info.minFilter = linear_filter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;

  sampler_info.addressModeU = repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT
                                     : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_info.addressModeV = repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT
                                     : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler_info.addressModeW = repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT
                                     : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  sampler_info.anisotropyEnable = VK_FALSE;
  sampler_info.maxAnisotropy    = 0;

  VkSampler sampler;
  if (vkCreateSampler(device->device, &sampler_info, nullptr, &sampler) !=
      VK_SUCCESS) {
    fatal("failed to create texture sampler!");
  }

  return sampler;
}

void bind_sampler(Device *device, VkDescriptorSet descriptor_set,
                  VkImageView image_view, VkSampler sampler, u32 binding,
                  u32 index = 0)
{
  VkDescriptorImageInfo image_info{};
  image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image_info.imageView   = image_view;
  image_info.sampler     = sampler;

  VkWriteDescriptorSet descriptor_write{};
  descriptor_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptor_write.dstSet          = descriptor_set;
  descriptor_write.dstBinding      = binding;
  descriptor_write.dstArrayElement = index;
  descriptor_write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptor_write.descriptorCount = 1;
  descriptor_write.pImageInfo      = &image_info;

  vkUpdateDescriptorSets(device->device, 1, &descriptor_write, 0, nullptr);
}

// struct Texture {
//   ImageBuffer image_buffer;
//   VkImageView image_view_ref;
//   VkSampler sampler_ref;

//   VkDescriptorSet desc_set;
// };

// Texture create_texture(Device *device)
// {
//   Texture tex;

//   tex.image_buffer   = create_image(device, 256, 256);
//   tex.image_view_ref = create_image_view(device, tex.image_buffer);
//   tex.sampler_ref    = create_sampler(device);

//   // VkDescriptorSet desc_set = gpu->create_descriptor_set(pipeline);
//   // vulkan.write_sampler(desc_set, image_view, sampler);

//   return tex;
// }
}  // namespace Gpu