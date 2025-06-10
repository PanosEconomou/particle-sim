// === vulkan_context.cpp ===
#include "vulkan_context.h"
#include <iostream>
#include <cstring>
#include <stdexcept>

static void check(VkResult result, const char* msg) {
    if (result != VK_SUCCESS) {
        std::cerr << "Error: " << msg << " (code " << result << ")\n";
        std::exit(EXIT_FAILURE);
    }
}

VulkanContext initVulkan() {
    VulkanContext ctx{};

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "ComputeApp";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "NoEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    check(vkCreateInstance(&instanceInfo, nullptr, &ctx.instance), "vkCreateInstance");

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, devices.data());

    for (auto dev : devices) {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &count, families.data());
        for (uint32_t i = 0; i < families.size(); ++i) {
            if (families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                ctx.physicalDevice = dev;
                ctx.computeQueueFamilyIndex = i;
                goto found;
            }
        }
    }
    throw std::runtime_error("No compute-capable device found");
found:

    float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = ctx.computeQueueFamilyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    check(vkCreateDevice(ctx.physicalDevice, &deviceInfo, nullptr, &ctx.device), "vkCreateDevice");

    vkGetDeviceQueue(ctx.device, ctx.computeQueueFamilyIndex, 0, &ctx.computeQueue);
    return ctx;
}

VulkanBuffer createBuffer(const VulkanContext& ctx, VkDeviceSize size, void* data) {
    VulkanBuffer buf{};
    buf.size = size;

    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    check(vkCreateBuffer(ctx.device, &info, nullptr, &buf.buffer), "vkCreateBuffer");

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(ctx.device, buf.buffer, &memReq);

    VkPhysicalDeviceMemoryProperties props;
    vkGetPhysicalDeviceMemoryProperties(ctx.physicalDevice, &props);

    uint32_t index = UINT32_MAX;
    for (uint32_t i = 0; i < props.memoryTypeCount; ++i) {
        if ((memReq.memoryTypeBits & (1 << i)) &&
            (props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
            (props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            index = i;
            break;
        }
    }
    if (index == UINT32_MAX) throw std::runtime_error("No suitable memory type");

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = index;
    check(vkAllocateMemory(ctx.device, &allocInfo, nullptr, &buf.memory), "vkAllocateMemory");

    vkBindBufferMemory(ctx.device, buf.buffer, buf.memory, 0);

    void* mapped;
    vkMapMemory(ctx.device, buf.memory, 0, size, 0, &mapped);
    memcpy(mapped, data, static_cast<size_t>(size));
    vkUnmapMemory(ctx.device, buf.memory);

    return buf;
}

void readBuffer(const VulkanContext& ctx, const VulkanBuffer& buf, void* dst) {
    void* mapped;
    vkMapMemory(ctx.device, buf.memory, 0, buf.size, 0, &mapped);
    memcpy(dst, mapped, static_cast<size_t>(buf.size));
    vkUnmapMemory(ctx.device, buf.memory);
}

void destroyBuffer(const VulkanContext& ctx, VulkanBuffer buf) {
    vkDestroyBuffer(ctx.device, buf.buffer, nullptr);
    vkFreeMemory(ctx.device, buf.memory, nullptr);
}

void cleanupVulkan(VulkanContext& ctx) {
    vkDestroyDevice(ctx.device, nullptr);
    vkDestroyInstance(ctx.instance, nullptr);
}

