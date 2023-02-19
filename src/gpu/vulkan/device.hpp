#pragma once

#include <vulkan/vulkan.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#pragma clang diagnostic pop

#include "containers/array.hpp"
#include "file.hpp"
#include "logging.hpp"
#include "platform.hpp"

namespace Gpu
{
struct Device {
  u32 current_image_index = 0;

  VkInstance vulkan_instance       = 0;
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkDevice device;

  u32 graphics_queue_family = 0;
  VkQueue graphics_queue;

  VkRenderPass render_pass;

  VkSurfaceKHR surface;
  VkSwapchainKHR swap_chain;

  Array<VkImage, 4> swap_chain_images;
  Array<VkImageView, 4> swap_chain_image_views;
  Array<VkFramebuffer, 4> swap_chain_framebuffers;
  VkFormat swap_chain_image_format;
  VkExtent2D swap_chain_extent;

  VkCommandPool command_pool;
  VkCommandBuffer command_buffer;

  VkDescriptorPool descriptor_pool;

  VmaAllocator vma_allocator;
};

Device init_device(Platform::VulkanExtensions vulkan_extensions,
                   b8 enable_validation = true)
{
  Device device;

  VkApplicationInfo app_info{};
  app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName   = "Fracas";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName        = "No Engine";
  app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion         = VK_API_VERSION_1_3;

  Array<const char *, 20> extensions;
  for (i32 i = 0; i < vulkan_extensions.count; i++) {
    extensions.push_back(vulkan_extensions.extensions[i]);
  }

  VkInstanceCreateInfo create_info{};
  create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo        = &app_info;
  create_info.enabledExtensionCount   = extensions.size;
  create_info.ppEnabledExtensionNames = extensions.data;
  create_info.enabledLayerCount       = 0;

  if (enable_validation) {
    const char *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};

    create_info.enabledLayerCount += 1;
    create_info.ppEnabledLayerNames = validation_layers;
  }

  if (vkCreateInstance(&create_info, nullptr, &device.vulkan_instance) !=
      VK_SUCCESS) {
    fatal("failed to create vulkan instance!");
  }

  // select physical device
  {
    u32 device_count = 0;
    vkEnumeratePhysicalDevices(device.vulkan_instance, &device_count, nullptr);
    if (device_count == 0) {
      fatal("failed to find GPUs with Vulkan support!");
    }
    VkPhysicalDevice vulkan_physical_devices[128];
    vkEnumeratePhysicalDevices(device.vulkan_instance, &device_count,
                               vulkan_physical_devices);
    device.physical_device = vulkan_physical_devices[0];
  }

  // select queue family
  {
    Array<VkQueueFamilyProperties, 128> queue_families;
    vkGetPhysicalDeviceQueueFamilyProperties(device.physical_device,
                                             &queue_families.size, nullptr);
    vkGetPhysicalDeviceQueueFamilyProperties(
        device.physical_device, &queue_families.size, queue_families.data);
    for (i32 i = 0; i < queue_families.size; i++) {
      VkQueueFamilyProperties queue_family = queue_families[i];
      if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        device.graphics_queue_family = i;
        break;
      }
    }
  }

  // create logical device
  {
    f32 queue_priorities[] = {1.f};
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = device.graphics_queue_family;
    queue_create_info.queueCount       = 1;
    queue_create_info.pQueuePriorities = queue_priorities;

    // this is for bbindless textures
    // TODO: check whether these features are available
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexing_features{};
    indexing_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    indexing_features.pNext                           = nullptr;
    indexing_features.descriptorBindingPartiallyBound = VK_TRUE;
    indexing_features.runtimeDescriptorArray          = VK_TRUE;

    VkDeviceCreateInfo create_info{};
    create_info.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos    = &queue_create_info;
    create_info.queueCreateInfoCount = 1;
    create_info.pEnabledFeatures     = nullptr;
    create_info.pNext                = &indexing_features;

    const char *extensions[]            = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    create_info.ppEnabledExtensionNames = extensions;
    create_info.enabledExtensionCount   = 1;

    if (vkCreateDevice(device.physical_device, &create_info, nullptr,
                       &device.device) != VK_SUCCESS) {
      fatal("failed to create logical device!");
    }
  }

  vkGetDeviceQueue(device.device, device.graphics_queue_family, 0,
                   &device.graphics_queue);

  VkCommandPoolCreateInfo pool_info{};
  pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = device.graphics_queue_family;
  if (vkCreateCommandPool(device.device, &pool_info, nullptr,
                          &device.command_pool) != VK_SUCCESS) {
    fatal("failed to create command pool!");
  }

  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = device.command_pool;
  alloc_info.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = 1;
  if (vkAllocateCommandBuffers(device.device, &alloc_info,
                               &device.command_buffer) != VK_SUCCESS) {
    fatal("failed to allocate command buffers!");
  }

  {
    VkDescriptorPoolSize storage_buffer_pool_size{};
    storage_buffer_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    storage_buffer_pool_size.descriptorCount = 1000;
    VkDescriptorPoolSize uniforms_pool_size{};
    uniforms_pool_size.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniforms_pool_size.descriptorCount = 1000;
    VkDescriptorPoolSize combined_sampler_pool_size{};
    combined_sampler_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    combined_sampler_pool_size.descriptorCount = 1001;

    VkDescriptorPoolSize pool_sizes[] = {storage_buffer_pool_size,
                                         uniforms_pool_size,
                                         combined_sampler_pool_size};

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
    pool_info.poolSizeCount = 3;
    pool_info.pPoolSizes    = pool_sizes;
    pool_info.maxSets       = 1000;

    if (vkCreateDescriptorPool(device.device, &pool_info, nullptr,
                               &device.descriptor_pool) != VK_SUCCESS) {
      fatal("failed to create descriptor pool!");
    }
  }

  {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice         = device.physical_device;
    allocatorInfo.device                 = device.device;
    allocatorInfo.instance               = device.vulkan_instance;
    vmaCreateAllocator(&allocatorInfo, &device.vma_allocator);
  }

  return device;
}

void init_swap_chain(Device *device)
{
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical_device,
                                            device->surface, &capabilities);
  info("minImageCount: ", capabilities.minImageCount,
       ", maxImageCount: ", capabilities.maxImageCount,
       ", currentExtent.w: ", capabilities.currentExtent.width,
       ", currentExtent.h: ", capabilities.currentExtent.height,
       ", supportedCompositeAlpha: ", capabilities.supportedCompositeAlpha);

  u32 formatCount;
  VkSurfaceFormatKHR formats[128];
  vkGetPhysicalDeviceSurfaceFormatsKHR(device->physical_device, device->surface,
                                       &formatCount, nullptr);
  if (formatCount != 0) {
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device->physical_device, device->surface, &formatCount, formats);
  }
  for (i32 i = 0; i < formatCount; i++) {
    info("format: ", i, ", format.colorspace: ", formats[i].colorSpace,
         " format.format: ", formats[i].format);
  }

  u32 presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      device->physical_device, device->surface, &presentModeCount, nullptr);
  VkPresentModeKHR present_modes[128];
  if (presentModeCount != 0) {
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical_device,
                                              device->surface,
                                              &presentModeCount, present_modes);
  }
  for (i32 i = 0; i < presentModeCount; i++) {
    info("presentMode", i, ": ", present_modes[i]);
  }

  VkSurfaceFormatKHR surface_format = {};
  surface_format.colorSpace =
      VkColorSpaceKHR::VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  surface_format.format         = VkFormat::VK_FORMAT_B8G8R8A8_UNORM;
  VkPresentModeKHR present_mode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
  VkExtent2D extent             = capabilities.currentExtent;

  device->swap_chain_image_format = surface_format.format;
  device->swap_chain_extent       = extent;

  VkSwapchainCreateInfoKHR create_info{};
  create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface          = device->surface;
  create_info.minImageCount    = 3;
  create_info.imageFormat      = surface_format.format;
  create_info.imageColorSpace  = surface_format.colorSpace;
  create_info.imageExtent      = extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.queueFamilyIndexCount = 0;
  create_info.pQueueFamilyIndices   = nullptr;
  create_info.presentMode           = present_mode;
  create_info.clipped               = VK_TRUE;
  create_info.oldSwapchain          = VK_NULL_HANDLE;
  create_info.preTransform          = capabilities.currentTransform;
  create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  if (vkCreateSwapchainKHR(device->device, &create_info, nullptr,
                           &device->swap_chain) != VK_SUCCESS) {
    fatal("failed to create swap chain");
  }

  u32 image_count;
  vkGetSwapchainImagesKHR(device->device, device->swap_chain, &image_count,
                          nullptr);
  device->swap_chain_images.resize(image_count);
  vkGetSwapchainImagesKHR(device->device, device->swap_chain, &image_count,
                          device->swap_chain_images.data);

  device->swap_chain_image_views.resize(device->swap_chain_images.size);
  for (i32 i = 0; i < device->swap_chain_images.size; i++) {
    VkImageViewCreateInfo create_info{};
    create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image    = device->swap_chain_images[i];
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format   = device->swap_chain_image_format;

    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel   = 0;
    create_info.subresourceRange.levelCount     = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(device->device, &create_info, nullptr,
                          &device->swap_chain_image_views[i]) != VK_SUCCESS) {
      fatal("failed to create image views!");
    }
  }
}
void destroy_swap_chain(Device *device)
{
  for (i32 i = 0; i < device->swap_chain_framebuffers.size; i++) {
    vkDestroyFramebuffer(device->device, device->swap_chain_framebuffers[i],
                         nullptr);
  }
  device->swap_chain_framebuffers.clear();
  for (i32 i = 0; i < device->swap_chain_image_views.size; i++) {
    vkDestroyImageView(device->device, device->swap_chain_image_views[i],
                       nullptr);
  }
  device->swap_chain_image_views.clear();
  vkDestroySwapchainKHR(device->device, device->swap_chain, nullptr);
}

void init_render_pass(Device *device)
{
  VkAttachmentDescription color_attachment{};
  color_attachment.format         = device->swap_chain_image_format;
  color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_ref{};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDependency dependency{};
  dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass    = 0;
  dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments    = &color_attachment_ref;

  VkRenderPassCreateInfo render_pass_info{};
  render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 1;
  render_pass_info.pAttachments    = &color_attachment;
  render_pass_info.subpassCount    = 1;
  render_pass_info.pSubpasses      = &subpass;
  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies   = &dependency;

  if (vkCreateRenderPass(device->device, &render_pass_info, nullptr,
                         &device->render_pass) != VK_SUCCESS) {
    fatal("failed to create render pass!");
  }
}

void create_framebuffers(Device *device)
{
  device->swap_chain_framebuffers.resize(device->swap_chain_image_views.size);
  for (u32 i = 0; i < device->swap_chain_framebuffers.size; i++) {
    VkImageView attachments[] = {device->swap_chain_image_views[i]};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = device->render_pass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments    = attachments;
    framebufferInfo.width           = device->swap_chain_extent.width;
    framebufferInfo.height          = device->swap_chain_extent.height;
    framebufferInfo.layers          = 1;

    if (vkCreateFramebuffer(device->device, &framebufferInfo, nullptr,
                            &device->swap_chain_framebuffers[i]) !=
        VK_SUCCESS) {
      fatal("failed to create framebuffer!");
    }
  }
}

void recreate_swap_chain(Device *device)
{
  vkDeviceWaitIdle(device->device);

  destroy_swap_chain(device);
  init_swap_chain(device);
  create_framebuffers(device);
}

VkSemaphore image_available_semaphore;
VkSemaphore render_finished_semaphore;
VkFence in_flight_fence;
void create_sync_objects(Device *device)
{
  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  if (vkCreateSemaphore(device->device, &semaphoreInfo, nullptr,
                        &image_available_semaphore) != VK_SUCCESS ||
      vkCreateSemaphore(device->device, &semaphoreInfo, nullptr,
                        &render_finished_semaphore) != VK_SUCCESS ||
      vkCreateFence(device->device, &fenceInfo, nullptr, &in_flight_fence) !=
          VK_SUCCESS) {
    fatal("failed to create semaphores!");
  }
}
void destroy_sync_objects(Device *device)
{
  vkDestroySemaphore(device->device, image_available_semaphore, nullptr);
  vkDestroySemaphore(device->device, render_finished_semaphore, nullptr);
  vkDestroyFence(device->device, in_flight_fence, nullptr);
}

Device init_vulkan(Platform::GlfwWindow *window)
{
  Device device = init_device(window->get_vulkan_extensions());
  glfwCreateWindowSurface(device.vulkan_instance, window->ref, nullptr,
                          &device.surface);
  init_swap_chain(&device);
  init_render_pass(&device);
  create_framebuffers(&device);
  create_sync_objects(&device);

  return device;
}

void destroy_vulkan(Device *device)
{
  vkDeviceWaitIdle(device->device);

  destroy_swap_chain(device);

  destroy_sync_objects(device);

  vkDestroyCommandPool(device->device, device->command_pool, nullptr);

  vkDestroyRenderPass(device->device, device->render_pass, nullptr);

  vkDestroySurfaceKHR(device->vulkan_instance, device->surface, nullptr);

  vkDestroyDevice(device->device, nullptr);
  vkDestroyInstance(device->vulkan_instance, nullptr);
}

u32 find_memory_type(Device *device, u32 type_filter,
                     VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(device->physical_device, &mem_properties);
  for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
    if ((type_filter & (1 << i)) &&
        (mem_properties.memoryTypes[i].propertyFlags & properties) ==
            properties) {
      return i;
    }
  }
  fatal("failed to find suitable memory type!");
  return -1;
};
}  // namespace Gpu