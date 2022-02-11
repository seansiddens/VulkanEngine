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
    glm::mat4 modelMatrix{1.0f};
    glm::mat4 normalMatrix{1.f};
};

SimpleRenderSystem::SimpleRenderSystem(VeDevice& device,
                                       VkRenderPass renderPass,
                                       VkDescriptorSetLayout globalSetLayout,
                                       VeGameObject::Map& gameObjects)
    : veDevice{device} {

    textureSampler = VeTexture::createTextureSampler(veDevice);

    numGameObjects = gameObjects.size();

    std::cout << "Number of game objects: " << numGameObjects << "\n";

    // Create descriptor pool which allows for a descriptor set for each game object.
    simplePool = VeDescriptorPool::Builder(veDevice)
                     .setMaxSets(numGameObjects)
                     .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, numGameObjects)
                     .build();

    // Create descriptor layout.
    simpleLayout =
        VeDescriptorSetLayout::Builder(veDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

    // Create descriptor set for each game object
    for (const auto& [id, obj] : gameObjects) {
            VkDescriptorSet descriptorSet{};

            // Write material info.
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = obj.texture->imageView();
            imageInfo.sampler = textureSampler;

            // Allocate and write descriptor set.
            VeDescriptorWriter(*simpleLayout, *simplePool)
                .writeImage(0, &imageInfo)
                .build(descriptorSet);

            // Insert into our map.
            objectDescriptorSets.emplace(id, descriptorSet);
        }

    std::cout << "# of object descriptor sets: " << objectDescriptorSets.size();

    createPipelineLayout(globalSetLayout);
    createPipeline(renderPass);
}

SimpleRenderSystem::~SimpleRenderSystem() {
    vkDestroySampler(veDevice.device(), textureSampler, nullptr);
    vkDestroyPipelineLayout(veDevice.device(), pipelineLayout, nullptr);
}

void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SimplePushConstantData);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout,
                                                            simpleLayout->getDescriptorSetLayout()};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
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
    vePipeline = std::make_unique<VePipeline>(veDevice,
                                              "shaders/textured_lit_mat.vert.spv",
                                              "shaders/textured_lit_mat.frag.spv",
                                              pipelineConfig);
}

void SimpleRenderSystem::renderGameObjects(FrameInfo& frameInfo) {
    // Bind the pipeline.
    vePipeline->bind(frameInfo.commandBuffer);

    // Only being bound once, not per object
    vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout,
                            0,
                            1,
                            &frameInfo.globalDescriptorSet,
                            0,
                            nullptr);

    // Render each game object.
    for (auto& kv : frameInfo.gameObjects) {
        auto id = kv.first;
        auto& obj = kv.second;

        // Push data containing model and normal matrix.
        SimplePushConstantData push{};
        push.modelMatrix = obj.transform.mat4();
        push.normalMatrix = obj.transform.normalMatrix();

        // Bind the descriptor set of the object we are rendering.
        vkCmdBindDescriptorSets(frameInfo.commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipelineLayout,
                                1,
                                1,
                                &objectDescriptorSets[id],
                                0,
                                nullptr);

        // Send push constants.
        vkCmdPushConstants(frameInfo.commandBuffer,
                           pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(SimplePushConstantData),
                           &push);

        obj.model->bind(frameInfo.commandBuffer);
        obj.model->draw(frameInfo.commandBuffer);
    }

    // throw std::runtime_error("Throwing error after rendering every game object!");
}

}  // namespace ve