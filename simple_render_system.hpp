#pragma once

// std
#include <memory>
#include <vector>

#include "ve_camera.hpp"
#include "ve_device.hpp"
#include "ve_game_object.hpp"
#include "ve_pipeline.hpp"

namespace ve {

class SimpleRenderSystem {
   public:
    SimpleRenderSystem(VeDevice &device, VkRenderPass renderPass);
    ~SimpleRenderSystem();

    // Remove copy constructors.
    SimpleRenderSystem(const SimpleRenderSystem &) = delete;
    SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

    void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<VeGameObject> &gameObjects,
                           const VeCamera &camera);

   private:
    void createPipelineLayout();
    void createPipeline(VkRenderPass renderPass);

    VeDevice &veDevice;

    std::unique_ptr<VePipeline> vePipeline;
    VkPipelineLayout pipelineLayout;
};

}  // namespace ve