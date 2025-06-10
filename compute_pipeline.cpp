// === compute_pipeline.cpp ===
#include "compute_pipeline.h"
#include <fstream>
#include <stdexcept>

static std::vector<char> readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) throw std::runtime_error("Failed to open shader file");
    size_t size = f.tellg();
    std::vector<char> buffer(size);
    f.seekg(0);
    f.read(buffer.data(), size);
    return buffer;
}

void runComputeShader(const VulkanContext& ctx,
                      const std::string& spirvPath,
                      const VulkanBuffer& buffer,
                      uint32_t elementCount) {
    auto code = readFile(spirvPath);

    VkShaderModuleCreateInfo smInfo{};
    smInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    smInfo.codeSize = code.size();
    smInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shader;
    vkCreateShaderModule(ctx.device, &smInfo, nullptr, &shader);

    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;

    VkDescriptorSetLayout setLayout;
    vkCreateDescriptorSetLayout(ctx.device, &layoutInfo, nullptr, &setLayout);

    VkPipelineLayoutCreateInfo pipeLayoutInfo{};
    pipeLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeLayoutInfo.setLayoutCount = 1;
    pipeLayoutInfo.pSetLayouts = &setLayout;

    VkPipelineLayout pipelineLayout;
    vkCreatePipelineLayout(ctx.device, &pipeLayoutInfo, nullptr, &pipelineLayout);

    VkPipelineShaderStageCreateInfo shaderStage{};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStage.module = shader;
    shaderStage.pName = "main";

    VkComputePipelineCreateInfo pipeInfo{};
    pipeInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeInfo.stage = shaderStage;
    pipeInfo.layout = pipelineLayout;

    VkPipeline pipeline;
    vkCreateComputePipelines(ctx.device, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &pipeline);

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    VkDescriptorPool descPool;
    vkCreateDescriptorPool(ctx.device, &poolInfo, nullptr, &descPool);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &setLayout;

    VkDescriptorSet descSet;
    vkAllocateDescriptorSets(ctx.device, &allocInfo, &descSet);

    VkDescriptorBufferInfo bufInfo{};
    bufInfo.buffer = buffer.buffer;
    bufInfo.offset = 0;
    bufInfo.range = buffer.size;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &bufInfo;

    vkUpdateDescriptorSets(ctx.device, 1, &write, 0, nullptr);

    VkCommandPoolCreateInfo cmdPoolInfo{};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = ctx.computeQueueFamilyIndex;

    VkCommandPool cmdPool;
    vkCreateCommandPool(ctx.device, &cmdPoolInfo, nullptr, &cmdPool);

    VkCommandBufferAllocateInfo cmdAlloc{};
    cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAlloc.commandPool = cmdPool;
    cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAlloc.commandBufferCount = 1;

    VkCommandBuffer cmdBuf;
    vkAllocateCommandBuffers(ctx.device, &cmdAlloc, &cmdBuf);

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(cmdBuf, &begin);
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descSet, 0, nullptr);
    vkCmdDispatch(cmdBuf, elementCount, 1, 1);
    vkEndCommandBuffer(cmdBuf);

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmdBuf;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence;
    vkCreateFence(ctx.device, &fenceInfo, nullptr, &fence);

    vkQueueSubmit(ctx.computeQueue, 1, &submit, fence);
    vkWaitForFences(ctx.device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(ctx.device, fence, nullptr);
    vkDestroyCommandPool(ctx.device, cmdPool, nullptr);
    vkDestroyDescriptorPool(ctx.device, descPool, nullptr);
    vkDestroyPipeline(ctx.device, pipeline, nullptr);
    vkDestroyPipelineLayout(ctx.device, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(ctx.device, setLayout, nullptr);
    vkDestroyShaderModule(ctx.device, shader, nullptr);
}

