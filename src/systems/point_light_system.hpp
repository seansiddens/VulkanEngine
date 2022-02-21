#pragma once

// std
#include <memory>
#include <vector>

#include "Core/ve_camera.hpp"
#include "Core/ve_frame_info.hpp"
#include "Core/ve_game_object.hpp"
#include "Renderer/ve_device.hpp"
#include "Renderer/ve_pipeline.hpp"

namespace ve {

class PointLightSystem {
   public:
    PointLightSystem(VeDevice &device,
                     VkRenderPass renderPass,
                     VkDescriptorSetLayout globalSetLayout);
    ~PointLightSystem();

    // Remove copy constructors.
    PointLightSystem(const PointLightSystem &) = delete;
    PointLightSystem &operator=(const PointLightSystem &) = delete;

    void render(FrameInfo &frameInfo);

   private:
    void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createPipeline(VkRenderPass renderPass);

   private:
    VeDevice &veDevice;

    std::unique_ptr<VePipeline> vePipeline;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
};

}  // namespace ve