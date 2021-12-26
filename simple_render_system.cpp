#include "simple_render_system.hpp"

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

// In the shaders, Vulkan expects the data to be aligned such that:
// - scalars aligned by N ( = 4 bytes given 32 bit floats)
// - vec2 aligned by 2N ( = 8 bytes)
// - vec3 and vec4 aligned by 4N ( = 16 bytes)
struct SimplePushConstantData {
    glm::mat2 transform{1.0f};
    glm::vec2 offset;
    alignas(16) glm::vec3 color;  // Align to 16 bytes.
};

SimpleRenderSystem::SimpleRenderSystem(VeDevice& device, VkRenderPass renderPass)
    : veDevice{device} {
    createPipelineLayout();
    createPipeline(renderPass);
}

SimpleRenderSystem::~SimpleRenderSystem() {
    vkDestroyPipelineLayout(veDevice.device(), pipelineLayout, nullptr);
}

void SimpleRenderSystem::createPipelineLayout() {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SimplePushConstantData);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(veDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before layout");

    PipelineConfigInfo pipelineConfig{};
    VePipeline::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    vePipeline = std::make_unique<VePipeline>(veDevice, "shaders/simple_shader.vert.spv",
                                              "shaders/simple_shader.frag.spv", pipelineConfig);
}

void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer,
                                           std::vector<VeGameObject>& gameObjects) {
    // update
    int i = 0;
    for (auto& obj : gameObjects) {
        i += 1;
        obj.transform2d.rotation =
            glm::mod<float>(obj.transform2d.rotation + 0.0001f * i, 2.f * glm::pi<float>());
    }

    // render
    vePipeline->bind(commandBuffer);

    for (auto& obj : gameObjects) {
        SimplePushConstantData push{};
        push.offset = obj.transform2d.translation;
        push.color = obj.color;
        push.transform = obj.transform2d.mat2();

        vkCmdPushConstants(commandBuffer, pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(SimplePushConstantData), &push);
        obj.model->bind(commandBuffer);
        obj.model->draw(commandBuffer);
    }
}

}  // namespace ve