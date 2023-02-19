#pragma once

#include <vulkan/vulkan.h>

#include "containers/array.hpp"
#include "file.hpp"
#include "gpu/vulkan/device.hpp"
#include "logging.hpp"
#include "math/math.hpp"
#include "platform.hpp"

namespace Gpu
{

enum struct Format {
  RG32F,
  RGB32F,
  RGBA32F,
};
VkFormat to_vk_format(Format f)
{
  switch (f) {
    case Format::RG32F:
      return VK_FORMAT_R32G32_SFLOAT;
    case Format::RGB32F:
      return VK_FORMAT_R32G32B32_SFLOAT;
    case Format::RGBA32F:
      return VK_FORMAT_R32G32B32A32_SFLOAT;
  }
}
i32 format_size(Format f)
{
  switch (f) {
    case Format::RG32F:
      return 8;
    case Format::RGB32F:
      return 12;
    case Format::RGBA32F:
      return 16;
  }
}

struct VertexDescription {
  Array<Format, 10> attributes;
};

struct ShadersDefinition {
  String vert_shader;
  String frag_shader;
};

struct VkVertexDescription {
  VkVertexInputBindingDescription binding_description;
  Array<VkVertexInputAttributeDescription, 10> vertex_attribute_descriptions;
};
VkVertexDescription get_vk_vertex_description(
    VertexDescription vertex_description)
{
  VkVertexDescription vk_vertex_description;

  vk_vertex_description.vertex_attribute_descriptions.resize(
      vertex_description.attributes.size);
  i32 offset = 0;
  for (i32 i = 0; i < vertex_description.attributes.size; i++) {
    vk_vertex_description.vertex_attribute_descriptions[i].binding  = 0;
    vk_vertex_description.vertex_attribute_descriptions[i].location = i;
    vk_vertex_description.vertex_attribute_descriptions[i].format =
        to_vk_format(vertex_description.attributes[i]);
    vk_vertex_description.vertex_attribute_descriptions[i].offset = offset;

    offset += format_size(vertex_description.attributes[i]);
  }

  vk_vertex_description.binding_description.binding = 0;
  vk_vertex_description.binding_description.inputRate =
      VK_VERTEX_INPUT_RATE_VERTEX;
  vk_vertex_description.binding_description.stride = offset;

  return vk_vertex_description;
}

VkShaderModule create_shader_module(VkDevice device, File file)
{
  VkShaderModuleCreateInfo create_info{};
  create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.pCode    = reinterpret_cast<const u32 *>(file.data.data);
  create_info.codeSize = file.data.size;

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &create_info, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    fatal("failed to create shader module!");
  }
  return shaderModule;
}

struct Pipeline {
  VkPipeline ref;
  VkPipelineLayout layout;

  Array<VkDescriptorSetLayout, 4> descriptor_set_layouts;
};

Array<VkDescriptorSetLayout, 4> create_dui_descriptor_set_layouts(Device *device)
{
  Array<VkDescriptorSetLayout, 4> layouts;

  Array<VkDescriptorBindingFlags, 2> binding_flags = {
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT, 
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT};
  VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extended_info{};
  extended_info.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
  extended_info.pNext         = nullptr;
  extended_info.bindingCount  = binding_flags.size;
  extended_info.pBindingFlags = binding_flags.data;

  VkDescriptorSetLayoutBinding sampler_layout_binding{};
  sampler_layout_binding.binding         = 0;
  sampler_layout_binding.descriptorCount = 1000;
  sampler_layout_binding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sampler_layout_binding.pImmutableSamplers = nullptr;
  sampler_layout_binding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding buffer_layout_binding{};
  buffer_layout_binding.binding            = 1;
  buffer_layout_binding.descriptorCount    = 1;
  buffer_layout_binding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  buffer_layout_binding.pImmutableSamplers = nullptr;
  buffer_layout_binding.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding bindings[] = {sampler_layout_binding,
                                             buffer_layout_binding};
  VkDescriptorSetLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.flags =
      VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
  layout_info.bindingCount = 2;
  layout_info.pBindings    = bindings;
  layout_info.pNext        = &extended_info;

  VkDescriptorSetLayout descriptor_set_layout;
  if (vkCreateDescriptorSetLayout(device->device, &layout_info, nullptr,
                                  &descriptor_set_layout) != VK_SUCCESS) {
    fatal("failed to create descriptor set layout!");
  }

  layouts.push_back(descriptor_set_layout);

  return layouts;
}

Pipeline create_pipeline(Device *device, ShadersDefinition shaders_definition,
                         Gpu::VertexDescription vertex_description,
                         VkRenderPass render_pass, u32 push_constant_size)
{
  Pipeline pipeline;

  VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                     VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamic_state{};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount = 2;
  dynamic_state.pDynamicStates    = dynamic_states;

  VkVertexDescription vertex_descriptions =
      get_vk_vertex_description(vertex_description);

  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount   = 0;
  vertex_input_info.pVertexBindingDescriptions      = nullptr;
  vertex_input_info.vertexAttributeDescriptionCount = 0;
  vertex_input_info.pVertexAttributeDescriptions    = nullptr;

  vertex_input_info.vertexBindingDescriptionCount = 1;
  vertex_input_info.pVertexBindingDescriptions =
      &vertex_descriptions.binding_description;
  vertex_input_info.vertexAttributeDescriptionCount =
      static_cast<u32>(vertex_descriptions.vertex_attribute_descriptions.size);
  vertex_input_info.pVertexAttributeDescriptions =
      vertex_descriptions.vertex_attribute_descriptions.data;

  VkPipelineInputAssemblyStateCreateInfo input_assembly{};
  input_assembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly.primitiveRestartEnable = VK_FALSE;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = {0, 0};

  VkViewport viewport{};
  viewport.x        = 0.0f;
  viewport.y        = 0.0f;
  viewport.width    = 0;
  viewport.height   = 0;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkPipelineViewportStateCreateInfo viewport_state{};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.pViewports    = &viewport;
  viewport_state.scissorCount  = 1;
  viewport_state.pScissors     = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
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
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable   = VK_FALSE;
  multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading      = 1.0f;      // Optional
  multisampling.pSampleMask           = nullptr;   // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
  multisampling.alphaToOneEnable      = VK_FALSE;  // Optional

  VkPipelineColorBlendAttachmentState color_blend_attachment{};
  color_blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable         = VK_TRUE;
  color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  color_blend_attachment.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
  color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo color_blending{};
  color_blending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable     = VK_FALSE;
  color_blending.logicOp           = VK_LOGIC_OP_COPY;
  color_blending.attachmentCount   = 1;
  color_blending.pAttachments      = &color_blend_attachment;
  color_blending.blendConstants[0] = 0.0f;
  color_blending.blendConstants[1] = 0.0f;
  color_blending.blendConstants[2] = 0.0f;
  color_blending.blendConstants[3] = 0.0f;

  pipeline.descriptor_set_layouts = create_dui_descriptor_set_layouts(device);

  VkPushConstantRange push_constant;
  push_constant.offset     = 0;
  push_constant.size       = push_constant_size;
  push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkPipelineLayoutCreateInfo pipeline_layout_info{};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = pipeline.descriptor_set_layouts.size;
  pipeline_layout_info.pSetLayouts    = pipeline.descriptor_set_layouts.data;
  pipeline_layout_info.pushConstantRangeCount = 1;
  pipeline_layout_info.pPushConstantRanges    = &push_constant;

  if (vkCreatePipelineLayout(device->device, &pipeline_layout_info, nullptr,
                             &pipeline.layout) != VK_SUCCESS) {
    fatal("failed to create pipeline layout!");
  }

  Temp tmp;
  File vert_shader_file = read_file(shaders_definition.vert_shader, &tmp);
  File frag_shader_file = read_file(shaders_definition.frag_shader, &tmp);

  VkShaderModule vert_shader_module =
      create_shader_module(device->device, vert_shader_file);
  VkShaderModule frag_shader_module =
      create_shader_module(device->device, frag_shader_file);

  VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
  vert_shader_stage_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vert_shader_stage_info.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vert_shader_stage_info.module = vert_shader_module;
  vert_shader_stage_info.pName  = "main";

  VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
  frag_shader_stage_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  frag_shader_stage_info.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  frag_shader_stage_info.module = frag_shader_module;
  frag_shader_stage_info.pName  = "main";

  VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info,
                                                     frag_shader_stage_info};

  VkGraphicsPipelineCreateInfo pipeline_info{};
  pipeline_info.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = 2;
  pipeline_info.pStages    = shader_stages;

  VkPipelineDepthStencilStateCreateInfo depth_stencil{};
  depth_stencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil.depthTestEnable       = VK_TRUE;
  depth_stencil.depthWriteEnable      = VK_TRUE;
  depth_stencil.depthCompareOp        = VK_COMPARE_OP_LESS;
  depth_stencil.depthBoundsTestEnable = VK_FALSE;
  depth_stencil.minDepthBounds        = 0.0f;
  depth_stencil.maxDepthBounds        = 1.0f;
  depth_stencil.stencilTestEnable     = VK_FALSE;
  depth_stencil.front                 = {};
  depth_stencil.back                  = {};

  pipeline_info.pVertexInputState   = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState      = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState   = &multisampling;
  pipeline_info.pDepthStencilState  = &depth_stencil;
  pipeline_info.pColorBlendState    = &color_blending;
  pipeline_info.pDynamicState       = &dynamic_state;

  pipeline_info.layout     = pipeline.layout;
  pipeline_info.renderPass = render_pass;
  pipeline_info.subpass    = 0;

  pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
  pipeline_info.basePipelineIndex  = -1;

  if (vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1,
                                &pipeline_info, nullptr,
                                &pipeline.ref) != VK_SUCCESS) {
    fatal("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(device->device, vert_shader_module, nullptr);
  vkDestroyShaderModule(device->device, frag_shader_module, nullptr);

  return pipeline;
}

void destroy_pipeline(Device *device, Pipeline pipeline)
{
  vkDestroyPipeline(device->device, pipeline.ref, nullptr);
  vkDestroyPipelineLayout(device->device, pipeline.layout, nullptr);
}

VkDescriptorSet create_descriptor_set(Device *device, Pipeline pipeline)
{
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool     = device->descriptor_pool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts        = pipeline.descriptor_set_layouts.data;

  VkDescriptorSet descriptor_set;
  if (vkAllocateDescriptorSets(device->device, &allocInfo, &descriptor_set) !=
      VK_SUCCESS) {
    fatal("failed to allocate descriptor sets!");
  }

  return descriptor_set;
}

void start_backbuffer(Device *device)
{
  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType      = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = device->render_pass;
  render_pass_info.framebuffer =
      device->swap_chain_framebuffers[device->current_image_index];
  render_pass_info.renderArea.offset = {0, 0};
  render_pass_info.renderArea.extent = device->swap_chain_extent;

  VkClearValue clear_color         = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
  render_pass_info.clearValueCount = 1;
  render_pass_info.pClearValues    = &clear_color;

  vkCmdBeginRenderPass(device->command_buffer, &render_pass_info,
                       VK_SUBPASS_CONTENTS_INLINE);
}

void end_backbuffer(Device *device)
{
  vkCmdEndRenderPass(device->command_buffer);
}

void start_command_buffer(Device *device)
{
  vkResetCommandBuffer(device->command_buffer, 0);

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(device->command_buffer, &begin_info) != VK_SUCCESS) {
    fatal("failed to begin recording command buffer!");
  }
}

void end_command_buffer(Device *device)
{
  if (vkEndCommandBuffer(device->command_buffer) != VK_SUCCESS) {
    fatal("failed to record command buffer!");
  }
}

void start_frame(Device *device)
{
  vkWaitForFences(device->device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
  vkResetFences(device->device, 1, &in_flight_fence);

  vkAcquireNextImageKHR(device->device, device->swap_chain, UINT64_MAX,
                        image_available_semaphore, VK_NULL_HANDLE,
                        &device->current_image_index);

  start_command_buffer(device);
}

void end_frame(Device *device)
{
  end_command_buffer(device);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[]      = {image_available_semaphore};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores    = waitSemaphores;
  submitInfo.pWaitDstStageMask  = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &device->command_buffer;

  VkSemaphore signal_semaphores[] = {render_finished_semaphore};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = signal_semaphores;

  if (vkQueueSubmit(device->graphics_queue, 1, &submitInfo, in_flight_fence) !=
      VK_SUCCESS) {
    fatal("failed to submit draw command buffer!");
  }

  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores    = signal_semaphores;

  VkSwapchainKHR swapChains[] = {device->swap_chain};
  present_info.swapchainCount = 1;
  present_info.pSwapchains    = swapChains;
  present_info.pImageIndices  = &device->current_image_index;
  present_info.pResults       = nullptr;  // Optional

  VkResult present_result =
      vkQueuePresentKHR(device->graphics_queue, &present_info);
  if (present_result == VK_ERROR_OUT_OF_DATE_KHR ||
      present_result == VK_SUBOPTIMAL_KHR) {
    recreate_swap_chain(device);
  }
}

void bind_pipeline(Device *device, Pipeline pipeline, Vec2u framebuffer_size)
{
  vkCmdBindPipeline(device->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline.ref);

  VkViewport viewport{};
  viewport.x        = 0.0f;
  viewport.y        = 0.0f;
  viewport.width    = (f32)framebuffer_size.x;
  viewport.height   = (f32)framebuffer_size.y;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(device->command_buffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = device->swap_chain_extent;
  vkCmdSetScissor(device->command_buffer, 0, 1, &scissor);
}

void bind_descriptor_set(Device *device, Pipeline pipeline,
                         VkDescriptorSet descriptor_set)
{
  vkCmdBindDescriptorSets(device->command_buffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0,
                          1, &descriptor_set, 0, nullptr);
}

void push_constant(Device *device, Pipeline pipeline, void *data, u32 size)
{
  vkCmdPushConstants(device->command_buffer, pipeline.layout,
                     VK_SHADER_STAGE_VERTEX_BIT, 0, size, data);
}

void set_scissor(Device *device, Rect rect)
{
  VkRect2D scissor{};
  scissor.offset = {(i32)rect.x, (i32)rect.y};
  scissor.extent = {(u32)rect.width, (u32)rect.height};
  vkCmdSetScissor(device->command_buffer, 0, 1, &scissor);
}

void bind_storage_buffer(Device *device, VkDescriptorSet descriptor_set,
                         Buffer buffer, u32 binding)
{
  VkDescriptorBufferInfo buffer_info{};
  buffer_info.buffer = buffer.ref;
  buffer_info.range  = buffer.size;
  buffer_info.offset = 0;

  VkWriteDescriptorSet descriptor_write{};
  descriptor_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptor_write.dstSet          = descriptor_set;
  descriptor_write.dstBinding      = binding;
  descriptor_write.dstArrayElement = 0;
  descriptor_write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptor_write.descriptorCount = 1;
  descriptor_write.pBufferInfo     = &buffer_info;

  vkUpdateDescriptorSets(device->device, 1, &descriptor_write, 0, nullptr);
}

}  // namespace Gpu