#pragma once

#include <vulkan/vulkan.h>

#include "containers/array.hpp"
#include "file.hpp"
#include "logging.hpp"
#include "platform.hpp"

struct Vulkan {
  struct Buffer {
    VkBuffer ref;
    VkDeviceMemory memory;
    u32 size;
  };
  Array<Buffer, 1024> buffers;

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

  VkPipelineLayout pipeline_layout;
  VkPipeline graphics_pipeline;

  VkCommandPool command_pool;
  VkCommandBuffer command_buffer;

  void init(Platform::GlfwWindow *window)
  {
    init_device(window->get_vulkan_extensions());
    glfwCreateWindowSurface(vulkan_instance, window->ref, nullptr, &surface);
    init_swap_chain();
    init_render_pass();
    create_framebuffers();
    create_sync_objects();
    create_pipeline();
  }

  void init_device(Platform::VulkanExtensions vulkan_extensions, b8 enable_validation = true)
  {
    VkApplicationInfo app_info{};
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName   = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName        = "No Engine";
    app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion         = VK_API_VERSION_1_3;

    VkInstanceCreateInfo create_info{};
    create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo        = &app_info;
    create_info.enabledExtensionCount   = vulkan_extensions.count;
    create_info.ppEnabledExtensionNames = vulkan_extensions.extensions;
    create_info.enabledLayerCount       = 0;

    if (enable_validation) {
      const char *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};

      create_info.enabledLayerCount += 1;
      create_info.ppEnabledLayerNames = validation_layers;
    }

    if (vkCreateInstance(&create_info, nullptr, &vulkan_instance) != VK_SUCCESS) {
      fatal("failed to create vulkan instance!");
    }

    // select physical device
    {
      u32 device_count = 0;
      vkEnumeratePhysicalDevices(vulkan_instance, &device_count, nullptr);
      if (device_count == 0) {
        fatal("failed to find GPUs with Vulkan support!");
      }
      VkPhysicalDevice vulkan_physical_devices[128];
      vkEnumeratePhysicalDevices(vulkan_instance, &device_count, vulkan_physical_devices);
      physical_device = vulkan_physical_devices[0];
    }

    // select queue family
    {
      u32 queue_family_count = 0;
      VkQueueFamilyProperties queue_families[128];
      vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                               queue_families);
      for (i32 i = 0; i < queue_family_count; i++) {
        VkQueueFamilyProperties queue_family = queue_families[i];
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
          graphics_queue_family = i;
          break;
        }
      }
    }

    // create logical device
    {
      f32 queue_priorities[] = {1.f};
      VkDeviceQueueCreateInfo queue_create_info{};
      queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_create_info.queueFamilyIndex = graphics_queue_family;
      queue_create_info.queueCount       = 1;
      queue_create_info.pQueuePriorities = queue_priorities;

      VkDeviceCreateInfo create_info{};
      create_info.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      create_info.pQueueCreateInfos    = &queue_create_info;
      create_info.queueCreateInfoCount = 1;
      create_info.pEnabledFeatures     = nullptr;

      const char *extensions[]            = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
      create_info.ppEnabledExtensionNames = extensions;
      create_info.enabledExtensionCount   = 1;

      if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
        fatal("failed to create logical device!");
      }
    }

    vkGetDeviceQueue(device, graphics_queue_family, 0, &graphics_queue);

    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = graphics_queue_family;
    if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS) {
      fatal("failed to create command pool!");
    }

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool        = command_pool;
    alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(device, &alloc_info, &command_buffer) != VK_SUCCESS) {
      fatal("failed to allocate command buffers!");
    }
  }

  void init_swap_chain()
  {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);
    info("minImageCount: ", capabilities.minImageCount,
         ", maxImageCount: ", capabilities.maxImageCount,
         ", currentExtent.w: ", capabilities.currentExtent.width,
         ", currentExtent.h: ", capabilities.currentExtent.height,
         ", supportedCompositeAlpha: ", capabilities.supportedCompositeAlpha);

    u32 formatCount;
    VkSurfaceFormatKHR formats[128];
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
      vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formatCount, formats);
    }
    for (i32 i = 0; i < formatCount; i++) {
      info("format: ", i, ", format.colorspace: ", formats[i].colorSpace,
           " format.format: ", formats[i].format);
    }

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &presentModeCount, nullptr);
    VkPresentModeKHR present_modes[128];
    if (presentModeCount != 0) {
      vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &presentModeCount,
                                                present_modes);
    }
    for (i32 i = 0; i < presentModeCount; i++) {
      info("presentMode", i, ": ", present_modes[i]);
    }

    VkSurfaceFormatKHR surface_format = {};
    surface_format.colorSpace         = VkColorSpaceKHR::VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    surface_format.format             = VkFormat::VK_FORMAT_B8G8R8A8_SRGB;
    VkPresentModeKHR present_mode     = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
    VkExtent2D extent                 = capabilities.currentExtent;

    swap_chain_image_format = surface_format.format;
    swap_chain_extent       = extent;

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface               = surface;
    create_info.minImageCount         = 2;
    create_info.imageFormat           = surface_format.format;
    create_info.imageColorSpace       = surface_format.colorSpace;
    create_info.imageExtent           = extent;
    create_info.imageArrayLayers      = 1;
    create_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices   = nullptr;
    create_info.presentMode           = present_mode;
    create_info.clipped               = VK_TRUE;
    create_info.oldSwapchain          = VK_NULL_HANDLE;
    create_info.preTransform          = capabilities.currentTransform;
    create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain) != VK_SUCCESS) {
      fatal("failed to create swap chain");
    }

    u32 image_count;
    vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
    swap_chain_images.resize(image_count);
    vkGetSwapchainImagesKHR(device, swap_chain, &image_count, swap_chain_images.data);

    swap_chain_image_views.resize(swap_chain_images.size);
    for (i32 i = 0; i < swap_chain_images.size; i++) {
      VkImageViewCreateInfo create_info{};
      create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      create_info.image    = swap_chain_images[i];
      create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      create_info.format   = swap_chain_image_format;

      create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

      create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      create_info.subresourceRange.baseMipLevel   = 0;
      create_info.subresourceRange.levelCount     = 1;
      create_info.subresourceRange.baseArrayLayer = 0;
      create_info.subresourceRange.layerCount     = 1;

      if (vkCreateImageView(device, &create_info, nullptr, &swap_chain_image_views[i]) !=
          VK_SUCCESS) {
        fatal("failed to create image views!");
      }
    }
  }
  void destroy_swap_chain()
  {
    for (i32 i = 0; i < swap_chain_framebuffers.size; i++) {
      vkDestroyFramebuffer(device, swap_chain_framebuffers[i], nullptr);
    }
    swap_chain_framebuffers.clear();
    for (i32 i = 0; i < swap_chain_image_views.size; i++) {
      vkDestroyImageView(device, swap_chain_image_views[i], nullptr);
    }
    swap_chain_image_views.clear();
    vkDestroySwapchainKHR(device, swap_chain, nullptr);
  }
  void recreate_swap_chain()
  {
    vkDeviceWaitIdle(device);

    destroy_swap_chain();
    init_swap_chain();
    create_framebuffers();
  }

  void init_render_pass()
  {
    VkAttachmentDescription color_attachment{};
    color_attachment.format         = swap_chain_image_format;
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

    if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS) {
      fatal("failed to create render pass!");
    }
  }

  void create_framebuffers()
  {
    swap_chain_framebuffers.resize(swap_chain_image_views.size);
    for (u32 i = 0; i < swap_chain_framebuffers.size; i++) {
      VkImageView attachments[] = {swap_chain_image_views[i]};

      VkFramebufferCreateInfo framebufferInfo{};
      framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass      = render_pass;
      framebufferInfo.attachmentCount = 1;
      framebufferInfo.pAttachments    = attachments;
      framebufferInfo.width           = swap_chain_extent.width;
      framebufferInfo.height          = swap_chain_extent.height;
      framebufferInfo.layers          = 1;

      if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swap_chain_framebuffers[i]) !=
          VK_SUCCESS) {
        fatal("failed to create framebuffer!");
      }
    }
  }

  VkSemaphore image_available_semaphore;
  VkSemaphore render_finished_semaphore;
  VkFence in_flight_fence;
  void create_sync_objects()
  {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &image_available_semaphore) !=
            VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &render_finished_semaphore) !=
            VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &in_flight_fence) != VK_SUCCESS) {
      fatal("failed to create semaphores!");
    }
  }
  void destroy_sync_objects()
  {
    vkDestroySemaphore(device, image_available_semaphore, nullptr);
    vkDestroySemaphore(device, render_finished_semaphore, nullptr);
    vkDestroyFence(device, in_flight_fence, nullptr);
  }

  void start_frame()
  {
    vkWaitForFences(device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &in_flight_fence);

    vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE,
                          &current_image_index);

    start_command_buffer();
  }

  void end_frame()
  {
    end_command_buffer();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[]      = {image_available_semaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount     = 1;
    submitInfo.pWaitSemaphores        = waitSemaphores;
    submitInfo.pWaitDstStageMask      = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &command_buffer;

    VkSemaphore signal_semaphores[] = {render_finished_semaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signal_semaphores;

    if (vkQueueSubmit(graphics_queue, 1, &submitInfo, in_flight_fence) != VK_SUCCESS) {
      fatal("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = signal_semaphores;

    VkSwapchainKHR swapChains[] = {swap_chain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains    = swapChains;
    present_info.pImageIndices  = &current_image_index;
    present_info.pResults       = nullptr;  // Optional

    VkResult present_result = vkQueuePresentKHR(graphics_queue, &present_info);
    if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR) {
      recreate_swap_chain();
    }
  }

  void start_command_buffer()
  {
    vkResetCommandBuffer(command_buffer, 0);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
      fatal("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass        = render_pass;
    render_pass_info.framebuffer       = swap_chain_framebuffers[current_image_index];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = swap_chain_extent;

    VkClearValue clear_color         = {{{0.7f, 0.9f, 0.3f, 1.0f}}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues    = &clear_color;

    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (f32)swap_chain_extent.width;
    viewport.height   = (f32)swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swap_chain_extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
  }

  void end_command_buffer()
  {
    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
      fatal("failed to record command buffer!");
    }
  }

  u32 findMemoryType(u32 type_filter, VkMemoryPropertyFlags properties)
  {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);
    for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
      if ((type_filter & (1 << i)) &&
          (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
        return i;
      }
    }
    fatal("failed to find suitable memory type!");
    return -1;
  };

  void upload_buffer(Buffer buffer, void *data, u32 size)
  {
    void *dest;
    vkMapMemory(device, buffer.memory, 0, size, 0, &dest);
    memcpy(dest, data, size);
    vkUnmapMemory(device, buffer.memory);
  }

  Buffer create_vertex_buffer(u32 size)
  {
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.size        = size;

    VkBuffer ref;
    if (vkCreateBuffer(device, &buffer_info, nullptr, &ref) != VK_SUCCESS) {
      fatal("failed to create vertex buffer!");
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device, ref, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex =
        findMemoryType(mem_requirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkDeviceMemory memory;
    if (vkAllocateMemory(device, &alloc_info, nullptr, &memory) != VK_SUCCESS) {
      fatal("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(device, ref, memory, 0);

    Buffer vb;
    vb.ref    = ref;
    vb.memory = memory;
    vb.size   = size;

    return vb;
  };

  struct VertexDescriptions {
    VkVertexInputBindingDescription binding_description;
    Array<VkVertexInputAttributeDescription, 10> vertex_attribute_descriptions;
  };
  VertexDescriptions get_vertex_descriptions()
  {
    VertexDescriptions vertex_descriptions;

    vertex_descriptions.binding_description.binding   = 0;
    vertex_descriptions.binding_description.stride    = 5 * sizeof(f32);
    vertex_descriptions.binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertex_descriptions.vertex_attribute_descriptions.resize(2);
    vertex_descriptions.vertex_attribute_descriptions[0].binding  = 0;
    vertex_descriptions.vertex_attribute_descriptions[0].location = 0;
    vertex_descriptions.vertex_attribute_descriptions[0].format   = VK_FORMAT_R32G32_SFLOAT;
    vertex_descriptions.vertex_attribute_descriptions[0].offset   = 0 * sizeof(f32);
    vertex_descriptions.vertex_attribute_descriptions[1].binding  = 0;
    vertex_descriptions.vertex_attribute_descriptions[1].location = 1;
    vertex_descriptions.vertex_attribute_descriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_descriptions.vertex_attribute_descriptions[1].offset   = 2 * sizeof(f32);

    return vertex_descriptions;
  }

  VkShaderModule create_shader_module(File file)
  {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = file.data.size;
    createInfo.pCode    = reinterpret_cast<const u32 *>(file.data.data);

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
      fatal("failed to create shader module!");
    }
    return shaderModule;
  }

  void create_pipeline()
  {
    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates    = dynamic_states;

    VertexDescriptions vertex_descriptions = get_vertex_descriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount   = 0;
    vertex_input_info.pVertexBindingDescriptions      = nullptr;
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    vertex_input_info.pVertexAttributeDescriptions    = nullptr;

    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions    = &vertex_descriptions.binding_description;
    vertex_input_info.vertexAttributeDescriptionCount =
        static_cast<u32>(vertex_descriptions.vertex_attribute_descriptions.size);
    vertex_input_info.pVertexAttributeDescriptions =
        vertex_descriptions.vertex_attribute_descriptions.data;

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swap_chain_extent;

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (f32)swap_chain_extent.width;
    viewport.height   = (f32)swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports    = &viewport;
    viewport_state.scissorCount  = 1;
    viewport_state.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
    rasterizer.depthBiasClamp          = 0.0f;  // Optional
    rasterizer.depthBiasSlopeFactor    = 0.0f;  // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = VK_FALSE;
    multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading      = 1.0f;      // Optional
    multisampling.pSampleMask           = nullptr;   // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
    multisampling.alphaToOneEnable      = VK_FALSE;  // Optional

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable         = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;       // Optional
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;       // Optional

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable     = VK_FALSE;
    color_blending.logicOp           = VK_LOGIC_OP_COPY;  // Optional
    color_blending.attachmentCount   = 1;
    color_blending.pAttachments      = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f;  // Optional
    color_blending.blendConstants[1] = 0.0f;  // Optional
    color_blending.blendConstants[2] = 0.0f;  // Optional
    color_blending.blendConstants[3] = 0.0f;  // Optional

    VkPushConstantRange push_constant;
    push_constant.offset     = 0;
    push_constant.size       = sizeof(Vec2f);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount         = 0;               // Optional
    pipeline_layout_info.pSetLayouts            = nullptr;         // Optional
    pipeline_layout_info.pushConstantRangeCount = 1;               // Optional
    pipeline_layout_info.pPushConstantRanges    = &push_constant;  // Optional

    if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) !=
        VK_SUCCESS) {
      fatal("failed to create pipeline layout!");
    }

    Temp tmp;
    File vert_shader_file = read_file("shaders/basic/vert.spv", &tmp);
    File frag_shader_file = read_file("shaders/basic/frag.spv", &tmp);

    VkShaderModule vert_shader_module = create_shader_module(vert_shader_file);
    VkShaderModule frag_shader_module = create_shader_module(frag_shader_file);

    VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
    vert_shader_stage_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName  = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
    frag_shader_stage_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName  = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info,
                                                       frag_shader_stage_info};

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages    = shader_stages;

    pipeline_info.pVertexInputState   = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState      = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState   = &multisampling;
    pipeline_info.pDepthStencilState  = nullptr;  // Optional
    pipeline_info.pColorBlendState    = &color_blending;
    pipeline_info.pDynamicState       = &dynamic_state;

    pipeline_info.layout     = pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass    = 0;

    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;  // Optional
    pipeline_info.basePipelineIndex  = -1;              // Optional

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
                                  &graphics_pipeline) != VK_SUCCESS) {
      throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, vert_shader_module, nullptr);
    vkDestroyShaderModule(device, frag_shader_module, nullptr);
  }

  void push_constant(void *data, u32 size)
  {
    vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, size, data);
  }

  void set_scissor(Rect rect)
  {
    VkRect2D scissor{};
    scissor.offset = {(i32)rect.x, (i32)rect.y};
    scissor.extent = {(u32)rect.width, (u32)rect.height};
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
  }

  void draw_vertex_buffer(Buffer b, u32 offset, u32 vert_count)
  {
    VkBuffer vertex_buffers[] = {b.ref};
    VkDeviceSize offsets[]    = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

    vkCmdDraw(command_buffer, vert_count, 1, offset, 0);
  };

  void destroy_buffer(Buffer b)
  {
    vkDestroyBuffer(device, b.ref, nullptr);
    vkFreeMemory(device, b.memory, nullptr);
  }

  void destroy()
  {
    vkDeviceWaitIdle(device);

    for (i32 i = 0; i < buffers.size; i++) {
      destroy_buffer(buffers[i]);
    }

    destroy_swap_chain();

    destroy_sync_objects();

    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);

    vkDestroyCommandPool(device, command_pool, nullptr);

    vkDestroyRenderPass(device, render_pass, nullptr);

    vkDestroySurfaceKHR(vulkan_instance, surface, nullptr);

    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(vulkan_instance, nullptr);
  }
};