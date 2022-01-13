#pragma once

#include <vulkan/vulkan.h>

#include "ve_camera.hpp"

namespace ve {

struct FrameInfo {
    int frameIndex;
    float frameTime;
    VkCommandBuffer commandBuffer;
    VeCamera &camera;
    VkDescriptorSet globalDescriptorSet;
};

}  // namespace ve