// === compute_pipeline.h ===
#pragma once
#include "vulkan_context.h"
#include <string>

void runComputeShader(const VulkanContext& ctx,
                      const std::string& spirvPath,
                      const VulkanBuffer& buffer,
                      uint32_t elementCount);
