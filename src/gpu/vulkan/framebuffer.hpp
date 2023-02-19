#pragma once

#include <vulkan/vulkan.h>

#include "gpu/vulkan/device.hpp"
#include "gpu/vulkan/texture.hpp"

namespace Gpu
{
struct Framebuffer {
  VkFramebuffer framebuffer;

  ImageBuffer color_image;
  VkImageView color_image_view;
  ImageBuffer depth_image;
  VkImageView depth_image_view;
  Vec2u size;

  VkRenderPass render_pass;
};

VkRenderPass create_framebuffer_render_pass(Device *device)
{
  VkAttachmentDescription attachments[2] = {};
  attachments[0].format                  = VK_FORMAT_R8G8B8A8_UNORM;
  attachments[0].samples                 = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  attachments[1].format         = VK_FORMAT_D32_SFLOAT_S8_UINT;
  attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
  attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference attachment_refs[2];
  attachment_refs[0].attachment = 0;
  attachment_refs[0].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachment_refs[1].attachment = 1;
  attachment_refs[1].layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDependency dependencies[2] = {};
  dependencies[0].srcSubpass          = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass          = 0;
  dependencies[0].srcStageMask        = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                 VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass    = 0;
  dependencies[1].dstSubpass    = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                 VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependencies[1].dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount    = 1;
  subpass.pColorAttachments       = &attachment_refs[0];
  subpass.pDepthStencilAttachment = &attachment_refs[1];

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 2;
  render_pass_info.pAttachments    = attachments;
  render_pass_info.subpassCount    = 1;
  render_pass_info.pSubpasses      = &subpass;
  render_pass_info.dependencyCount = 2;
  render_pass_info.pDependencies   = dependencies;

  VkRenderPass render_pass;
  if (vkCreateRenderPass(device->device, &render_pass_info, nullptr,
                         &render_pass) != VK_SUCCESS) {
    fatal("failed to create render pass!");
  }

  return render_pass;
}

Framebuffer create_framebuffer(Device *device, Vec2u size)
{
  Framebuffer fb;
  fb.size        = size;
  fb.render_pass = create_framebuffer_render_pass(device);

  {
    VkImageCreateInfo image_info{};
    image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType     = VK_IMAGE_TYPE_2D;
    image_info.extent.width  = size.x;
    image_info.extent.height = size.y;
    image_info.extent.depth  = 1;
    image_info.mipLevels     = 1;
    image_info.arrayLayers   = 1;
    image_info.samples       = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags         = 0;
    image_info.format        = VK_FORMAT_R8G8B8A8_UNORM;
    image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device->device, &image_info, nullptr,
                      &fb.color_image.image) != VK_SUCCESS) {
      fatal("failed to create image!");
    }

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(device->device, fb.color_image.image,
                                 &mem_requirements);
    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex =
        find_memory_type(device, mem_requirements.memoryTypeBits,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(device->device, &alloc_info, nullptr,
                         &fb.color_image.memory) != VK_SUCCESS) {
      fatal("failed to allocate memory!");
    }
    vkBindImageMemory(device->device, fb.color_image.image,
                      fb.color_image.memory, 0);

    VkImageViewCreateInfo view_create_info{};
    view_create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.image    = fb.color_image.image;
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_create_info.format   = VK_FORMAT_R8G8B8A8_UNORM;
    view_create_info.subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT;
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount   = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount     = 1;

    VkImageView image_view;
    if (vkCreateImageView(device->device, &view_create_info, nullptr,
                          &fb.color_image_view) != VK_SUCCESS) {
      fatal("failed to create image view!");
    }

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter        = VK_FILTER_LINEAR;
    sampler_info.minFilter        = VK_FILTER_LINEAR;
    sampler_info.addressModeU     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW     = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy    = 0;

    // if (vkCreateSampler(device->device, &sampler_info, nullptr, &sampler) !=
    //     VK_SUCCESS) {
    //   fatal("failed to create texture sampler!");
    // }
  }

  {
    VkImageCreateInfo image_info{};
    image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType     = VK_IMAGE_TYPE_2D;
    image_info.extent.width  = size.x;
    image_info.extent.height = size.y;
    image_info.extent.depth  = 1;
    image_info.mipLevels     = 1;
    image_info.arrayLayers   = 1;
    image_info.samples       = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags         = 0;
    image_info.format        = VK_FORMAT_D32_SFLOAT_S8_UINT;
    image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device->device, &image_info, nullptr,
                      &fb.depth_image.image) != VK_SUCCESS) {
      fatal("failed to create image!");
    }

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(device->device, fb.depth_image.image,
                                 &mem_requirements);
    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex =
        find_memory_type(device, mem_requirements.memoryTypeBits,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(device->device, &alloc_info, nullptr,
                         &fb.depth_image.memory) != VK_SUCCESS) {
      fatal("failed to allocate memory!");
    }
    vkBindImageMemory(device->device, fb.depth_image.image,
                      fb.depth_image.memory, 0);

    VkImageViewCreateInfo view_create_info{};
    view_create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.image    = fb.depth_image.image;
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_create_info.format   = VK_FORMAT_D32_SFLOAT_S8_UINT;
    view_create_info.subresourceRange.aspectMask   = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount   = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount     = 1;

    VkImageView image_view;
    if (vkCreateImageView(device->device, &view_create_info, nullptr,
                          &fb.depth_image_view) != VK_SUCCESS) {
      fatal("failed to create image view!");
    }
  }

  VkImageView attachments[2] = {fb.color_image_view, fb.depth_image_view};
  VkFramebufferCreateInfo framebuffer_create_info;
  framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebuffer_create_info.renderPass      = fb.render_pass;
  framebuffer_create_info.attachmentCount = 2;
  framebuffer_create_info.pAttachments    = attachments;
  framebuffer_create_info.width           = size.x;
  framebuffer_create_info.height          = size.y;
  framebuffer_create_info.layers          = 1;
  if (vkCreateFramebuffer(device->device, &framebuffer_create_info, nullptr,
                          &fb.framebuffer) != VK_SUCCESS) {
    fatal("failed to create image view!");
  }

  return fb;
}

void start_framebuffer(Device *device, Framebuffer framebuffer)
{
  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass        = framebuffer.render_pass;
  render_pass_info.framebuffer       = framebuffer.framebuffer;
  render_pass_info.renderArea.offset = {0, 0};
  render_pass_info.renderArea.extent = {framebuffer.size.x, framebuffer.size.y};

  VkClearValue clear_colors[2]     = {};
  clear_colors[0].color            = {0.1f, 0.1f, 0.1f, 1.0f};
  clear_colors[1].depthStencil     = {1.f, 0};
  render_pass_info.clearValueCount = 2;
  render_pass_info.pClearValues    = clear_colors;

  vkCmdBeginRenderPass(device->command_buffer, &render_pass_info,
                       VK_SUBPASS_CONTENTS_INLINE);
}

void end_framebuffer(Device *device)
{
  vkCmdEndRenderPass(device->command_buffer);
}
}  // namespace Gpu