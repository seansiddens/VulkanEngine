#include "first_app.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

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

FirstApp::FirstApp() {
    loadGameObjects();
    createPipelineLayout();
    recreateSwapChain();
    createCommandBuffers();
}

FirstApp::~FirstApp() { vkDestroyPipelineLayout(veDevice.device(), pipelineLayout, nullptr); }

void FirstApp::run() {
    // Check the maximum size of push constants available on the device.
    std::cout << "maxPushConstantsSize  = " << veDevice.properties.limits.maxPushConstantsSize
              << "\n";

    while (!veWindow.shouldClose()) {
        glfwPollEvents();
        drawFrame();
    }

    // Wait for GPU to finish before exiting.
    vkDeviceWaitIdle(veDevice.device());
}

void FirstApp::loadGameObjects() {
    std::vector<VeModel::Vertex> vertices = {{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                             {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                             {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};
    auto veModel = std::make_shared<VeModel>(veDevice, vertices);

    auto triangle = VeGameObject::createGameObject();
    triangle.model = veModel;
    triangle.color = {.1f, .8f, .1f};
    triangle.transform2d.translation.x = .2f;

    gameObjects.push_back(std::move(triangle));
}

void FirstApp::createPipelineLayout() {
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

void FirstApp::createPipeline() {
    assert(veSwapChain != nullptr && "Cannot create swap chain before pipeline!");
    assert(pipelineLayout != nullptr && "Cannot create pipeline before layout");

    PipelineConfigInfo pipelineConfig{};
    VePipeline::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = veSwapChain->getRenderPass();
    pipelineConfig.pipelineLayout = pipelineLayout;
    vePipeline = std::make_unique<VePipeline>(veDevice, "shaders/simple_shader.vert.spv",
                                              "shaders/simple_shader.frag.spv", pipelineConfig);
}

void FirstApp::createCommandBuffers() {
    // We are allocating a command buffer for each of our framebuffers.
    commandBuffers.resize(veSwapChain->imageCount());

    // Allocate the command buffers.
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    // Only primary cmd buffers can be submitted.
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = veDevice.getCommandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(veDevice.device(), &allocInfo, commandBuffers.data()) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers");
    }
}

void FirstApp::freeCommandBuffers() {
    vkFreeCommandBuffers(veDevice.device(), veDevice.getCommandPool(),
                         static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    commandBuffers.clear();
}

void FirstApp::drawFrame() {
    // Fetch the index of the image we are rendering to next.
    uint32_t imageIndex;
    auto result = veSwapChain->acquireNextImage(&imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Error occurs if window has been resized.
        recreateSwapChain();
        return;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire next swap chain image!");
    }

    recordCommandBuffer(imageIndex);
    // Sumbit command buffer to the device graphics queue. Swap chain will submit the image to be
    // rendered to.
    result = veSwapChain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        veWindow.wasWindowResized()) {
        // Every frame we are checking if the window was resized.
        veWindow.resetWindowResizedFlag();
        recreateSwapChain();
        return;
    }
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
}

void FirstApp::recreateSwapChain() {
    auto extent = veWindow.getExtent();
    // Ensure dimensions aren't zero width/height.
    while (extent.width == 0 || extent.height == 0) {
        extent = veWindow.getExtent();
        glfwWaitEvents();
    }

    // Wait until current swapchain is no longer being used before we create the new one.
    vkDeviceWaitIdle(veDevice.device());

    if (veSwapChain == nullptr) {
        // Create new swap chain.
        veSwapChain = std::make_unique<VeSwapChain>(veDevice, extent);
    } else {
        // Constructs a swap chain w/ a pointer to the previous one.
        veSwapChain = std::make_unique<VeSwapChain>(veDevice, extent, std::move(veSwapChain));
        if (veSwapChain->imageCount() != commandBuffers.size()) {
            freeCommandBuffers();
            createCommandBuffers();
        }
    }

    // TODO: If pipeline is compatible, pipeline does not have to be recreated.
    createPipeline();  // Pipeline depends on swap chain so it must be remade.
}

void FirstApp::recordCommandBuffer(int imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // Begin recording.
    if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = veSwapChain->getRenderPass();
    renderPassInfo.framebuffer = veSwapChain->getFrameBuffer(imageIndex);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = veSwapChain->getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.01f, 0.015f, 0.02f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    // VK_SUBPASS_INLINE indicates that subsequent commands are recorded directly into the
    // primary command buffer. No secondary command buffers are used.
    vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Create viewport.
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(veSwapChain->getSwapChainExtent().width);
    viewport.height = static_cast<float>(veSwapChain->getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, veSwapChain->getSwapChainExtent()};
    vkCmdSetViewport(commandBuffers[imageIndex], 0, 1, &viewport);
    vkCmdSetScissor(commandBuffers[imageIndex], 0, 1, &scissor);

    renderGameObjects(commandBuffers[imageIndex]);

    // Finish recording commands
    vkCmdEndRenderPass(commandBuffers[imageIndex]);
    if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void FirstApp::renderGameObjects(VkCommandBuffer commandBuffer) {
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