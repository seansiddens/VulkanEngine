#include "first_app.hpp"

#include <array>
#include <iostream>
#include <stdexcept>

namespace ve {

FirstApp::FirstApp() {
    loadModels();
    createPipelineLayout();
    createPipeline();
    createCommandBuffers();
}

FirstApp::~FirstApp() { vkDestroyPipelineLayout(veDevice.device(), pipelineLayout, nullptr); }

void FirstApp::run() {
    std::cout << "Width: " << veSwapChain.width() << "\n";
    std::cout << "Height: " << veSwapChain.height() << "\n";

    while (!veWindow.shouldClose()) {
        glfwPollEvents();
        drawFrame();
    }

    // Wait for GPU to finish before exiting.
    vkDeviceWaitIdle(veDevice.device());
}

void FirstApp::createPipelineLayout() {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;
    if (vkCreatePipelineLayout(veDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void FirstApp::createPipeline() {
    auto pipelineConfig =
        VePipeline::defaultPipelineConfigInfo(veSwapChain.width(), veSwapChain.height());
    pipelineConfig.renderPass = veSwapChain.getRenderPass();
    pipelineConfig.pipelineLayout = pipelineLayout;
    vePipeline = std::make_unique<VePipeline>(veDevice, "shaders/simple_shader.vert.spv",
                                              "shaders/simple_shader.frag.spv", pipelineConfig);
}

void FirstApp::createCommandBuffers() {
    // We are allocating a command buffer for each of our framebuffers.
    commandBuffers.resize(veSwapChain.imageCount());

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

    // We are recording commands only once at initialization instead of every frame.
    for (int i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        // Begin recording.
        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = veSwapChain.getRenderPass();
        renderPassInfo.framebuffer = veSwapChain.getFrameBuffer(i);

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = veSwapChain.getSwapChainExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.1f, 0.15f, 0.2f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        // VK_SUBPASS_INLINE indicates that subsequent commands are recorded directly into the
        // primary command buffer. No secondary command buffers are used.
        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Bind our pipeline.
        vePipeline->bind(commandBuffers[i]);

        // Draw.
        veModel->bind(commandBuffers[i]);
        veModel->draw(commandBuffers[i]);

        // Finish recording commands
        vkCmdEndRenderPass(commandBuffers[i]);
        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
};

void FirstApp::drawFrame() {
    // Fetch the index of the image we are rendering to next.
    uint32_t imageIndex;
    auto result = veSwapChain.acquireNextImage(&imageIndex);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire next swap chain image!");
    }

    // Sumbit command buffer to the device graphics queue. Swap chain will submit the image to be
    // rendered to.
    result = veSwapChain.submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
};

void FirstApp::loadModels() {
    std::vector<VeModel::Vertex> vertices = {{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                             {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                             {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};

    veModel = std::make_unique<VeModel>(veDevice, vertices);
}

}  // namespace ve