#pragma once

#include "Renderer/ve_device.hpp"

// std
#include <memory>

namespace ve {

class VeTexture {
   public:
    VeTexture(VeDevice& device, const std::string& filepath);
    VeTexture(VeDevice& device, const std::vector<unsigned char>& pixels, int texWidth, int texHeight);
    ~VeTexture();

    static std::unique_ptr<VeTexture> createTextureFromFile(VeDevice& device,
                                                            const std::string& filepath);
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
    VkImage textureImage{};
    VkDeviceMemory textureImageMemory{};
    VkImageView textureImageView{};
};

}  // namespace ve