#pragma once

namespace Gpu
{
namespace Vulkan
{

struct PerFrameData {
  f32 time;
};

struct PerPassData {
  Vec3f camera_position;
};

VkDescriptorSetLayout create_per_frame_descriptor_set_layout(Device *device)
{
  VkDescriptorSetLayoutBinding per_frame_data_layout_binding{};
  per_frame_data_layout_binding.binding            = 0;
  per_frame_data_layout_binding.descriptorCount    = 1;
  per_frame_data_layout_binding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  per_frame_data_layout_binding.pImmutableSamplers = nullptr;
  per_frame_data_layout_binding.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding bindings[] = {per_frame_data_layout_binding};
  VkDescriptorSetLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.flags =
      VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
  layout_info.bindingCount = 1;
  layout_info.pBindings    = bindings;
  layout_info.pNext        = nullptr;

  VkDescriptorSetLayout descriptor_set_layout;
  if (vkCreateDescriptorSetLayout(device->device, &layout_info, nullptr,
                                  &descriptor_set_layout) != VK_SUCCESS) {
    fatal("failed to create descriptor set layout!");
  }

  return descriptor_set_layout;
}

VkDescriptorSetLayout create_per_pass_descriptor_set_layout(Device *device)
{
  VkDescriptorSetLayoutBinding per_pass_data_layout_binding{};
  per_pass_data_layout_binding.binding            = 0;
  per_pass_data_layout_binding.descriptorCount    = 1;
  per_pass_data_layout_binding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  per_pass_data_layout_binding.pImmutableSamplers = nullptr;
  per_pass_data_layout_binding.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding bindings[] = {per_pass_data_layout_binding};
  VkDescriptorSetLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.flags =
      VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
  layout_info.bindingCount = 1;
  layout_info.pBindings    = bindings;
  layout_info.pNext        = nullptr;

  VkDescriptorSetLayout descriptor_set_layout;
  if (vkCreateDescriptorSetLayout(device->device, &layout_info, nullptr,
                                  &descriptor_set_layout) != VK_SUCCESS) {
    fatal("failed to create descriptor set layout!");
  }

  return descriptor_set_layout;
}

VkDescriptorSetLayout create_per_shader_model_descriptor_set_layout(
    Device *device)
{
  VkDescriptorSetLayoutBinding per_shader_model_data_layout_binding{};
  per_shader_model_data_layout_binding.binding            = 0;
  per_shader_model_data_layout_binding.descriptorCount    = 1;
  per_shader_model_data_layout_binding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  per_shader_model_data_layout_binding.pImmutableSamplers = nullptr;
  per_shader_model_data_layout_binding.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding sampler_layout_binding{};
  sampler_layout_binding.binding         = 1;
  sampler_layout_binding.descriptorCount = 1;
  sampler_layout_binding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sampler_layout_binding.pImmutableSamplers = nullptr;
  sampler_layout_binding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding bindings[] = {
      per_shader_model_data_layout_binding, sampler_layout_binding};
  VkDescriptorSetLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.flags =
      VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
  layout_info.bindingCount = 2;
  layout_info.pBindings    = bindings;
  layout_info.pNext        = nullptr;

  VkDescriptorSetLayout descriptor_set_layout;
  if (vkCreateDescriptorSetLayout(device->device, &layout_info, nullptr,
                                  &descriptor_set_layout) != VK_SUCCESS) {
    fatal("failed to create descriptor set layout!");
  }

  return descriptor_set_layout;
}

VkDescriptorSetLayout create_per_material_descriptor_set_layout(
    Device *device, i32 num_textures)
{
  VkDescriptorSetLayoutBinding sampler_layout_binding{};
  sampler_layout_binding.binding         = 0;
  sampler_layout_binding.descriptorCount = num_textures;
  sampler_layout_binding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sampler_layout_binding.pImmutableSamplers = nullptr;
  sampler_layout_binding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding bindings[] = {sampler_layout_binding};
  VkDescriptorSetLayoutCreateInfo layout_info{};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.flags =
      VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
  layout_info.bindingCount = 1;
  layout_info.pBindings    = bindings;
  layout_info.pNext        = nullptr;

  VkDescriptorSetLayout descriptor_set_layout;
  if (vkCreateDescriptorSetLayout(device->device, &layout_info, nullptr,
                                  &descriptor_set_layout) != VK_SUCCESS) {
    fatal("failed to create descriptor set layout!");
  }

  return descriptor_set_layout;
}

Array<VkDescriptorSetLayout, 4> create_descriptor_set_layouts(Device *device,
                                                              i32 num_textures)
{
  Array<VkDescriptorSetLayout, 4> layouts;
  layouts.push_back(create_per_frame_descriptor_set_layout(device));
  layouts.push_back(create_per_pass_descriptor_set_layout(device));
  layouts.push_back(create_per_shader_model_descriptor_set_layout(device));
  layouts.push_back(
      create_per_material_descriptor_set_layout(device, num_textures));
  return layouts;
}
}  // namespace Vulkan
}  // namespace Gpu