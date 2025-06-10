// A lightweight Vulkan compute pipeline wrapper
// ---------------------------------------------
// Files:
//   - main.cpp              (entry point)
//   - vulkan_context.h/.cpp (instance, device, buffer)
//   - compute_pipeline.h/.cpp (shader, descriptor set, dispatch)

// === vulkan_context.h ===
#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

struct VulkanContext {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue computeQueue;
    uint32_t computeQueueFamilyIndex;
};

struct VulkanBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDeviceSize size;
};

VulkanContext initVulkan();
VulkanBuffer createBuffer(const VulkanContext&, VkDeviceSize, void* data);
VulkanBuffer createUniformBuffer(const VulkanContext&, VkDeviceSize, void* data);
void readBuffer(const VulkanContext&, const VulkanBuffer&, void* dst);
void cleanupVulkan(VulkanContext&);
void destroyBuffer(const VulkanContext&, VulkanBuffer);

