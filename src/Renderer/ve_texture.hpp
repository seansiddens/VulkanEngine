#pragma once

#include "Renderer/ve_device.hpp"

// std
#include <memory>

namespace ve {

class VeTexture {
   public:
    VeTexture(VeDevice& device, const std::string& filepath, VkFormat = VK_FORMAT_R8G8B8A8_SRGB);
    VeTexture(VeDevice& device,
              const std::vector<unsigned char>& pixels,
              int texWidth,
              int texHeight,
              VkFormat = VK_FORMAT_R8G8B8A8_SRGB);
    ~VeTexture();

    static std::unique_ptr<VeTexture> createTextureFromFile(
        VeDevice& device, const std::string& filepath, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);

    static std::unique_ptr<VeTexture> createCubemapFromFile(VeDevice&);
    // Creates a 1x1 white texture.
    static std::unique_ptr<VeTexture> createEmptyTexture(VeDevice&);
    static VkSampler createTextureSampler(VeDevice& veDevice);

    [[nodiscard]] VkImageView imageView() const { return textureImageView; }

   private:
    void createTextureImageFromFile(const std::string& filepath);
    void createTextureImageFromPixels(const std::vector<unsigned char>& pixels,
                                      int texWidth,
                                      int texHeight);
    void createTextureImageView();

    VeDevice& veDevice;
    VkFormat m_format{VK_FORMAT_R8G8B8A8_SRGB};
    VkImage textureImage{};
    VkDeviceMemory textureImageMemory{};
    VkImageView textureImageView{};
};

}  // namespace ve