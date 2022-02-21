#pragma once

// std
#include <string>
#include <vector>

#include "Core/ve_window.hpp"

namespace ve {

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    uint32_t graphicsFamily{};
    uint32_t presentFamily{};
    bool graphicsFamilyHasValue = false;
    bool presentFamilyHasValue = false;
    [[nodiscard]] bool isComplete() const {
        return graphicsFamilyHasValue && presentFamilyHasValue;
    }
};

class VeDevice {
   public:
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    explicit VeDevice(VeWindow &veWindow);
    ~VeDevice();

    // Not copyable or movable
    VeDevice(const VeDevice &) = delete;
    VeDevice &operator=(const VeDevice &) = delete;
    VeDevice(VeDevice &&) = delete;
    VeDevice &operator=(VeDevice &&) = delete;

    VkCommandPool getCommandPool() { return commandPool; }
    VkDevice device() { return device_; }
    VkSurfaceKHR surface() { return surface_; }
    VkQueue graphicsQueue() { return graphicsQueue_; }
    VkQueue presentQueue() { return presentQueue_; }
    VkInstance getInstance() { return instance; }
    VkPhysicalDevice getPhysicalDevice() { return physicalDevice; }

    SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }
    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                                 VkImageTiling tiling,
                                 VkFormatFeatureFlags features);

    // Buffer helper functions
    void createBuffer(VkDeviceSize size,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties,
                      VkBuffer &buffer,
                      VkDeviceMemory &bufferMemory);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(
        VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

    void createImageWithInfo(const VkImageCreateInfo &imageInfo,
                             VkMemoryPropertyFlags properties,
                             VkImage &image,
                             VkDeviceMemory &imageMemory);

    void transitionImageLayout(VkImage image,
                               VkFormat format,
                               VkImageLayout oldLayout,
                               VkImageLayout newLayout);
   public:
    VkPhysicalDeviceProperties properties{};

   private:
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createCommandPool();

    // Helper functions.
    bool isDeviceSuitable(VkPhysicalDevice device);
    [[nodiscard]] std::vector<const char *> getRequiredExtensions() const;
    bool checkValidationLayerSupport();
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    void hasGflwRequiredInstanceExtensions();
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

   private:
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VeWindow &veWindow;
    VkCommandPool commandPool{};  // Command buffers are allocated from this memory.

    VkDevice device_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;

    const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};

}  // namespace ve