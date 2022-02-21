#pragma once

#include <vulkan/vulkan.h>

#include <memory>

#include "ve_device.hpp"

namespace ve {

class VeSwapChain {
   public:
    // Maximum number of command buffers that can be submitted to the graphics queue at once.
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    VeSwapChain(VeDevice &deviceRef, VkExtent2D windowExtent);
    VeSwapChain(VeDevice &deviceRef,
                VkExtent2D windowExtent,
                std::shared_ptr<VeSwapChain> previous);
    ~VeSwapChain();

    VeSwapChain(const VeSwapChain &) = delete;
    VeSwapChain &operator=(const VeSwapChain &) = delete;

    // A renderpass instance renders to the memory attachments contained within a framebuffer
    // object. In this case the framebuffer attachments are owned by the swapchain, but this
    // is not mandatory. Instead, we could render to a texture by attaching our own images
    // which could then be sampled from a shader.
    VkFramebuffer getFrameBuffer(int index) { return swapChainFramebuffers[index]; }
    VkRenderPass getRenderPass() { return renderPass; }
    VkImageView getImageView(int index) { return swapChainImageViews[index]; }
    size_t imageCount() { return swapChainImages.size(); }  // Number of framebuffers.
    VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
    VkExtent2D getSwapChainExtent() { return swapChainExtent; }
    [[nodiscard]] uint32_t width() const { return swapChainExtent.width; }
    [[nodiscard]] uint32_t height() const { return swapChainExtent.height; }

    [[nodiscard]] float extentAspectRatio() const {
        return static_cast<float>(swapChainExtent.width) /
               static_cast<float>(swapChainExtent.height);
    }
    VkFormat findDepthFormat();

    VkResult acquireNextImage(uint32_t *imageIndex);
    VkResult submitCommandBuffers(const VkCommandBuffer *buffers, const uint32_t *imageIndex);

    [[nodiscard]] bool compareSwapFormats(const VeSwapChain &swapChain) const {
        return swapChain.swapChainDepthFormat == swapChainDepthFormat &&
               swapChain.swapChainImageFormat == swapChainImageFormat;
    }

   private:
    void init();
    void createSwapChain();
    void createImageViews();
    void createDepthResources();
    void createRenderPass();
    void createFramebuffers();
    void createSyncObjects();

    // Helper functions
    static VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    VkFormat swapChainImageFormat;
    VkFormat swapChainDepthFormat;
    VkExtent2D swapChainExtent;

    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkRenderPass renderPass;

    std::vector<VkImage> depthImages;
    std::vector<VkDeviceMemory> depthImageMemorys;
    std::vector<VkImageView> depthImageViews;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    VeDevice &veDevice;
    VkExtent2D windowExtent;

    VkSwapchainKHR swapChain;
    std::shared_ptr<VeSwapChain> oldSwapChain;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;
};

}  // namespace ve