#pragma once

#include <vulkan/vulkan.h>

#include "ve_camera.hpp"
#include "ve_game_object.hpp"

namespace ve {

struct FrameInfo {
    int frameIndex;
    float frameTime;
    VkCommandBuffer commandBuffer;
    VeCamera &camera;
    VkDescriptorSet globalDescriptorSet;
    VeGameObject::Map &gameObjects;
};

}  // namespace ve