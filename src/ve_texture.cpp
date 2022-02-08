#include "ve_texture.hpp"

#include "ve_buffer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Pathing is done from the build directory, so we define a macro to orient us automatically
// in the project root directory.
#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace ve {

VeTexture::VeTexture(VeDevice& device, const std::string& filepath) : veDevice{device} {
    createTextureImage(filepath);
    createTextureImageView();
}

VeTexture::~VeTexture() {
    vkDestroyImageView(veDevice.device(), textureImageView, nullptr);
    vkDestroyImage(veDevice.device(), textureImage, nullptr);
    vkFreeMemory(veDevice.device(), textureImageMemory, nullptr);
}

std::unique_ptr<VeTexture> VeTexture::createTextureFromFile(VeDevice& device,
                                                            const std::string& filepath) {
    return std::make_unique<VeTexture>(device, filepath);
}

void VeTexture::createTextureImage(const std::string& filepath) {
    std::string enginePath = ENGINE_DIR + filepath;
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels =
        stbi_load(enginePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VeBuffer imageStagingBuffer(
        veDevice,
        sizeof(pixels[0]),
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    imageStagingBuffer.map();
    imageStagingBuffer.writeToBuffer(pixels);
    imageStagingBuffer.unmap();

    stbi_image_free(pixels);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(texWidth);
    imageInfo.extent.height = static_cast<uint32_t>(texHeight);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // Going to be used as destination of our staging buffer and will be sampled in our shaders.
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // idk what this does.
    // Used for multisampling.
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    // Create the image.
    veDevice.createImageWithInfo(
        imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

    // The image data is currently stored in the staging buffer, so we must copy it to our image.
    // Transition the image layout to be optimal for transfering data.
    veDevice.transitionImageLayout(textureImage,
                                   VK_FORMAT_R8G8B8A8_SRGB,
                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy from staging buffer to the image
    veDevice.copyBufferToImage(imageStagingBuffer.getBuffer(),
                               textureImage,
                               static_cast<uint32_t>(texWidth),
                               static_cast<uint32_t>(texHeight),
                               1);

    // Transition the image layout to be optimal for sampling.
    veDevice.transitionImageLayout(textureImage,
                                   VK_FORMAT_R8G8B8A8_SRGB,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void VeTexture::createTextureImageView() {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = textureImage;  // Image associated w/ the view.
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(veDevice.device(), &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }
}

VkSampler VeTexture::createTextureSampler(VeDevice& veDevice) {
    VkSamplerCreateInfo samplerInfo{};

    // Filtering settings.
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;  // Oversampling.
    samplerInfo.minFilter = VK_FILTER_LINEAR;  // Undersampling.

    // How to handle addressing out of bounds of the image.
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = veDevice.properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkSampler textureSampler{};
    if (vkCreateSampler(veDevice.device(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    return textureSampler;
}

}  // namespace ve