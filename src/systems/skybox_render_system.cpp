#include "skybox_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace ve {

SkyboxSystem::SkyboxSystem(VeDevice& device,
                                   VkRenderPass renderPass,
                                   VkDescriptorSetLayout globalSetLayout,
                                   std::shared_ptr<VeTexture> cubemap)
    : veDevice{device}, m_cubemap{cubemap} {
    
    // Create sampler for cubemap.
    m_cubemapSampler = VeTexture::createTextureSampler(veDevice);

    // Create descriptor pool for cubemap.
    m_cubemapPool = VeDescriptorPool::Builder(veDevice)
                        .setMaxSets(1)
                        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
                        .build();
    
    // Create descriptor set layout for cubemap.
    m_cubemapLayout = VeDescriptorSetLayout::Builder(veDevice)
                        .addBinding(0, 
                                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                    VK_SHADER_STAGE_FRAGMENT_BIT)
                        .build();
    
    // Cubemap image info.
    VkDescriptorImageInfo cubemapInfo{};
    cubemapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    cubemapInfo.imageView = cubemap->imageView();
    cubemapInfo.sampler = m_cubemapSampler;

    // Allocate and write descriptor set.
    VeDescriptorWriter(*m_cubemapLayout, *m_cubemapPool)
        .writeImage(0, &cubemapInfo)
        .build(m_cubemapDescriptorSet);

    createPipelineLayout(globalSetLayout);
    createPipeline(renderPass);
}

SkyboxSystem::~SkyboxSystem() {
    vkDestroySampler(veDevice.device(), m_cubemapSampler, nullptr);
    vkDestroyPipelineLayout(veDevice.device(), pipelineLayout, nullptr);
}

void SkyboxSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    // pushConstantRange.size = sizeof(SimplePushConstantData);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout,
                                                            m_cubemapLayout->getDescriptorSetLayout()};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;
    if (vkCreatePipelineLayout(veDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void SkyboxSystem::createPipeline(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before layout");

    PipelineConfigInfo pipelineConfig{};
    VePipeline::defaultPipelineConfigInfo(pipelineConfig);

    // Cubemap vertices are hard-coded into the shader so we don't need a vertex buffer.
    pipelineConfig.bindingDescriptions.clear();
    pipelineConfig.attributeDescriptions.clear();
    
    // Skybox will always have a depth of 1.0, so if we always draw the skybox last, and 
    // set the depth test function to pass when values are less or equal to depth buffer's content,
    // then the skybox will only be drawn when needed.
    pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_EQUAL;

    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    vePipeline = std::make_unique<VePipeline>(
        veDevice, "assets/shaders/skybox.vert.spv", "assets/shaders/skybox.frag.spv", pipelineConfig);
}

void SkyboxSystem::renderSkybox(FrameInfo& frameInfo) {
    // Bind the pipeline.
    vePipeline->bind(frameInfo.commandBuffer);

    // Bind global descriptor set as set 0.
    vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout,
                            0,
                            1,
                            &frameInfo.globalDescriptorSet,
                            0,
                            nullptr);

    // Bind cubemap descriptor as set 1.
    vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout,
                            1,
                            1,
                            &m_cubemapDescriptorSet,
                            0,
                            nullptr);

    // Draw.
    vkCmdDraw(frameInfo.commandBuffer, 36, 1, 0, 0);
}

}  // namespace ve