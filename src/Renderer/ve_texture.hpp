#pragma once

#include "Renderer/ve_device.hpp"

// std
#include <memory>

namespace ve {

class VeTexture {
   public:
    VeTexture(VeDevice& device, const std::string& filepath);
    ~VeTexture();

    static std::unique_ptr<VeTexture> createTextureFromFile(VeDevice& device,
                                                            const std::string& filepath);
    static VkSampler createTextureSampler(VeDevice &veDevice);

    VkImageView imageView() const { return textureImageView; }

   private:
    void createTextureImage(const std::string& filepath);
    void createTextureImageView();

    VeDevice& veDevice;
    VkImage textureImage{};
    VkDeviceMemory textureImageMemory{};
    VkImageView textureImageView{};
};

}  // namespace ve