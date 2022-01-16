#include "ve_renderer.hpp"

namespace ve {

VeRenderer::VeRenderer(VeWindow &window, VeDevice &device) : veWindow{window}, veDevice{device} {
    recreateSwapChain();
    createCommandBuffers();
}

VeRenderer::~VeRenderer() { freeCommandBuffers(); }

VkCommandBuffer VeRenderer::beginFrame() {
    assert(!isFrameStarted && "Can't call beginFrame while frame already in progress!");

    // Fetch the next swap chain image.
    auto result = veSwapChain->acquireNextImage(&currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Error occurs if window has been resized.
        recreateSwapChain();
        return nullptr;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire next swap chain image!");
    }

    // Successfully acquired next image - start recording new command buffer.
    isFrameStarted = true;
    auto commandBuffer = getCurrentCommandBuffer();
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // Begin recording.
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    return commandBuffer;
}

void VeRenderer::endFrame() {
    assert(isFrameStarted && "Can't call endFrame() while frame is not in progress!");

    // End command buffer.
    auto commandBuffer = getCurrentCommandBuffer();
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to end command buffer!");
    }

    // Submit command buffer to begin execution on GPU.
    auto result = veSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        veWindow.wasWindowResized()) {
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    isFrameStarted = false;
    currentFrameIndex = (currentFrameIndex + 1) % VeSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void VeRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
    assert(isFrameStarted && "Can't call beginSwapChainRenderPass() if frame not in progress!");
    assert(commandBuffer == getCurrentCommandBuffer() &&
           "Can't begin render pass on command buffer from a different frame");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = veSwapChain->getRenderPass();
    renderPassInfo.framebuffer = veSwapChain->getFrameBuffer(currentImageIndex);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = veSwapChain->getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.00f, 0.0f, 0.00f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    // VK_SUBPASS_INLINE indicates that subsequent commands are recorded directly into the
    // primary command buffer. No secondary command buffers are used.
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Create viewport.
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(veSwapChain->getSwapChainExtent().width);
    viewport.height = static_cast<float>(veSwapChain->getSwapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, veSwapChain->getSwapChainExtent()};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}
void VeRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
    assert(isFrameStarted && "Can't call endSwapChainRenderPass() if frame not in progress!");
    assert(commandBuffer == getCurrentCommandBuffer() &&
           "Can't end render pass on command buffer from a different frame");

    vkCmdEndRenderPass(commandBuffer);
}

void VeRenderer::createCommandBuffers() {
    // We are allocating a command buffer for each of our framebuffers.
    commandBuffers.resize(VeSwapChain::MAX_FRAMES_IN_FLIGHT);

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

void VeRenderer::freeCommandBuffers() {
    vkFreeCommandBuffers(veDevice.device(), veDevice.getCommandPool(),
                         static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    commandBuffers.clear();
}

void VeRenderer::recreateSwapChain() {
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
        std::shared_ptr<VeSwapChain> oldSwapChain = std::move(veSwapChain);
        // Constructs a swap chain w/ a pointer to the previous one.
        veSwapChain = std::make_unique<VeSwapChain>(veDevice, extent, oldSwapChain);

        if (!oldSwapChain->compareSwapFormats(*veSwapChain.get())) {
            throw std::runtime_error("Swap chain image/depth format has changed!");
        }
    }
}

}  // namespace ve