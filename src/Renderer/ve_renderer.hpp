#pragma once


#include "Core/ve_window.hpp"
#include "Renderer/ve_device.hpp"
#include "Renderer/ve_swap_chain.hpp"

// std
#include <cassert>
#include <memory>
#include <vector>

namespace ve {

class VeRenderer {
   public:
    VeRenderer(VeWindow &window, VeDevice &device);
    ~VeRenderer();

    // Remove copy constructors.
    VeRenderer(const VeRenderer &) = delete;
    VeRenderer &operator=(const VeRenderer &) = delete;

    VkCommandBuffer beginFrame();
    void endFrame();

    void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
    void endSwapChainRenderPass(VkCommandBuffer commandBuffer) const;

    [[nodiscard]] VkRenderPass getSwapChainRenderPass() const {
        return veSwapChain->getRenderPass();
    }
    [[nodiscard]] float getAspectRatio() const { return veSwapChain->extentAspectRatio(); }
    [[nodiscard]] bool isFrameInProgress() const { return isFrameStarted; }
    [[nodiscard]] VkCommandBuffer getCurrentCommandBuffer() const {
        assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
        return commandBuffers[currentFrameIndex];
    }
    [[nodiscard]] int getFrameIndex() const {
        assert(isFrameStarted && "Cannot get frame index when frame not in progress!");
        return currentFrameIndex;
    }
    [[nodiscard]] uint32_t getSwapChainImageCount() const { return veSwapChain->imageCount(); }
    [[nodiscard]] VeWindow& getWindow() const { return veWindow; }
    [[nodiscard]] VeDevice& getDevice() const { return veDevice; }

    void setClearColor(VkClearColorValue _clearColor) { clearColor = _clearColor; }

   private:
    void createCommandBuffers();
    void freeCommandBuffers();
    void recreateSwapChain();

    VeWindow &veWindow;
    VeDevice &veDevice;
    std::unique_ptr<VeSwapChain> veSwapChain;
    std::vector<VkCommandBuffer> commandBuffers;

    VkClearColorValue clearColor{0.f, 0.f, 0.f, 1.f};
    uint32_t currentImageIndex{0};
    int currentFrameIndex{0};
    bool isFrameStarted{false};
};

}  // namespace ve