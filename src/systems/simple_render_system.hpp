#pragma once

// std
#include <vulkan/vulkan.h>

#include <memory>
#include <unordered_map>
#include <vector>

#include "ve_camera.hpp"
#include "ve_descriptors.hpp"
#include "ve_device.hpp"
#include "ve_frame_info.hpp"
#include "ve_game_object.hpp"
#include "ve_pipeline.hpp"
#include "ve_swap_chain.hpp"

namespace ve {

class SimpleRenderSystem {
   public:
    SimpleRenderSystem(VeDevice &device,
                       VkRenderPass renderPass,
                       VkDescriptorSetLayout globalSetLayout,
                       VeGameObject::Map &gameObjects);
    ~SimpleRenderSystem();

    // Remove copy constructors.
    SimpleRenderSystem(const SimpleRenderSystem &) = delete;
    SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

    void renderGameObjects(FrameInfo &frameInfo);

   private:
    void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createPipeline(VkRenderPass renderPass);

    VeDevice &veDevice;

    // Number of game objects we are rendering.
    int numGameObjects;

    std::unique_ptr<VePipeline> vePipeline;
    VkPipelineLayout pipelineLayout;

    std::unique_ptr<VeDescriptorPool> simplePool{};
    std::unique_ptr<VeDescriptorSetLayout> simpleLayout{};

    // Each game object has a descriptor set.
    // Map from game object ID to descriptor set.
    std::unordered_map<id_t, VkDescriptorSet> objectDescriptorSets;

    // Sampler for game object's textures.
    VkSampler textureSampler{};
};

}  // namespace ve