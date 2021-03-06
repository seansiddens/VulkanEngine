#pragma once

// std
#include <array>
#include <memory>
#include <vector>

#include "Core/ve_camera.hpp"
#include "Core/ve_frame_info.hpp"
#include "Core/ve_game_object.hpp"
#include "Renderer/ve_device.hpp"
#include "Renderer/ve_descriptors.hpp"
#include "Renderer/ve_pipeline.hpp"

namespace ve {

class SkyboxSystem {
   public:
    SkyboxSystem(VeDevice &device,
                     VkRenderPass renderPass,
                     VkDescriptorSetLayout globalSetLayout,
                     std::shared_ptr<VeTexture> cubemap);
    ~SkyboxSystem();

    // Remove copy constructors.
    SkyboxSystem(const SkyboxSystem &) = delete;
    SkyboxSystem &operator=(const SkyboxSystem &) = delete;

    void renderSkybox(FrameInfo &frameInfo);

   private:
    void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createPipeline(VkRenderPass renderPass);

   private:
    VeDevice &veDevice;

    std::unique_ptr<VeDescriptorPool> m_cubemapPool;
    std::unique_ptr<VeDescriptorSetLayout> m_cubemapLayout;
    VkDescriptorSet m_cubemapDescriptorSet;

    std::shared_ptr<VeTexture> m_cubemap;
    VkSampler m_cubemapSampler;

    std::unique_ptr<VePipeline> vePipeline;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
};

}  // namespace ve