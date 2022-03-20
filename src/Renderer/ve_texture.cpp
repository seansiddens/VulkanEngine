#include "ve_texture.hpp"

#include "Renderer/ve_buffer.hpp"

// lib
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// std
#include <stdexcept>

// Pathing is done from the build directory, so we define a macro to orient us automatically
// in the project root directory.
#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace ve {

VeTexture::VeTexture(VeDevice& device, const std::string& filepath, VkFormat format, bool isCubemap)
    : veDevice{device}, m_format{format} {
    if (!isCubemap) {
        createTextureImageFromFile(filepath);
        createTextureImageView();
    } else {
        createCubemapImageFromFile(filepath);
        createCubemapImageView();
    }
}

VeTexture::VeTexture(VeDevice& device,
                     const std::vector<unsigned char>& pixels,
                     int texWidth,
                     int texHeight,
                     VkFormat format)
    : veDevice{device}, m_format{format} {
    createTextureImageFromPixels(pixels, texWidth, texHeight);
    createTextureImageView();
}


VeTexture::~VeTexture() {
    vkDestroyImageView(veDevice.device(), textureImageView, nullptr);
    vkDestroyImage(veDevice.device(), textureImage, nullptr);
    vkFreeMemory(veDevice.device(), textureImageMemory, nullptr);
}


std::unique_ptr<VeTexture> VeTexture::createTextureFromFile(VeDevice& device,
                                                            const std::string& filepath,
                                                            VkFormat format) {
    return std::make_unique<VeTexture>(device, filepath, format, false);
}

std::unique_ptr<VeTexture> VeTexture::createCubemapFromFile(VeDevice &device, 
                                                            const std::string& filepath,
                                                            VkFormat format) {
    return std::make_unique<VeTexture>(device, filepath, format, true);
}

std::unique_ptr<VeTexture> VeTexture::createEmptyTexture(VeDevice& veDevice) {
    unsigned char pixels[] = {255, 255, 255, 255};  // Texture data.
    return std::make_unique<VeTexture>(
        veDevice, std::vector<unsigned char>(std::begin(pixels), std::end(pixels)), 1, 1, VK_FORMAT_R8G8B8A8_UNORM);
}

// Creates VkImage and VkDeviceMemory for a cubemap with data loaded from disk.
// filepath is expected to be a folder holding textures for:
//      back, bottom, front, left, right, and top
// https://satellitnorden.wordpress.com/2018/01/23/vulkan-adventures-cube-map-tutorial/
void VeTexture::createCubemapImageFromFile(const std::string& filepath) {
    // Each texture is loaded separately and stored in an array.
    char *textureData[6];
    int width{0};
    int height{0};
    int numOfChannels{0};

    // Load each texture individually.
    std::string enginePath = ENGINE_DIR + filepath;
    textureData[0] = (char *)stbi_load((enginePath + "front.jpg").c_str(), &width, &height, &numOfChannels, STBI_rgb_alpha);
    textureData[1] = (char *)stbi_load((enginePath + "back.jpg").c_str(), &width, &height, &numOfChannels, STBI_rgb_alpha);
    textureData[2] = (char *)stbi_load((enginePath + "up.jpg").c_str(), &width, &height, &numOfChannels, STBI_rgb_alpha);
    textureData[3] = (char *)stbi_load((enginePath + "down.jpg").c_str(), &width, &height, &numOfChannels, STBI_rgb_alpha);
    textureData[4] = (char *)stbi_load((enginePath + "right.jpg").c_str(), &width, &height, &numOfChannels, STBI_rgb_alpha);
    textureData[5] = (char *)stbi_load((enginePath + "left.jpg").c_str(), &width, &height, &numOfChannels, STBI_rgb_alpha);
    // Check that everything was successfully loaded.
    for (auto pixels : textureData) {
        if (!pixels) {
            throw std::runtime_error("failed to load cubemap image!");
        }
    }

    //Calculate the image size and the layer size.
    const VkDeviceSize imageSize = width * height * 4 * 6; 
    const VkDeviceSize layerSize = imageSize / 6; //This is just the size of each layer.

    // Create staging buffer and write pixel data to it.
    VeBuffer imageStagingBuffer(
        veDevice,
        sizeof(textureData[0][0]),
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    imageStagingBuffer.map();
    // Write data to the staging buffer.
    for (int i = 0; i < 6; i++) {
        imageStagingBuffer.writeToBuffer(textureData[i], layerSize, layerSize * i);
        stbi_image_free(textureData[i]); // Free pixel data.
    }
    imageStagingBuffer.unmap();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6; // Cubemap
    imageInfo.format = m_format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // Going to be used as destination of our staging buffer and will be sampled in our shaders.
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // idk what this does.
    // Used for multisampling.
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = VK_IMAGE_VIEW_TYPE_CUBE; // Cubemap

    // Initializes the VkImage member.
    veDevice.createImageWithInfo(
        imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

    // The image data is currently stored in the staging buffer, so we must copy it to our image.
    // Transition the image layout to be optimal for transfering data.
    veDevice.transitionImageLayout(textureImage,
                                   VK_FORMAT_R8G8B8A8_SRGB,
                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   6); // Need to transition all 6 layers.

    // Copy from staging buffer to the image
    veDevice.copyBufferToImage(imageStagingBuffer.getBuffer(),
                               textureImage,
                               static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height),
                               6);

    // Transition the image layout to be optimal for sampling.
    veDevice.transitionImageLayout(textureImage,
                                   VK_FORMAT_R8G8B8A8_SRGB,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


}

// Initializes the VkImage member struct bound with VkDeviceMemory holding the texture data loaded
// from disk.
void VeTexture::createTextureImageFromFile(const std::string& filepath) {
    std::string enginePath = ENGINE_DIR + filepath;
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels =
        stbi_load(enginePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    // Create staging buffer and write pixel data to it.
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
    imageInfo.format = m_format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // Going to be used as destination of our staging buffer and will be sampled in our shaders.
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // idk what this does.
    // Used for multisampling.
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    // Initializes the VkImage member.
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

void VeTexture::createTextureImageFromPixels(const std::vector<unsigned char>& pixels,
                                             int texWidth,
                                             int texHeight) {
    auto* pixelMemory = (unsigned char*)pixels.data();
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    // Create staging buffer.
    VeBuffer imageStagingBuffer(
        veDevice,
        sizeof(pixelMemory[0]),
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Write texture data to staging buffer.
    imageStagingBuffer.map();
    imageStagingBuffer.writeToBuffer(pixelMemory);
    imageStagingBuffer.unmap();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(texWidth);
    imageInfo.extent.height = static_cast<uint32_t>(texHeight);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = m_format;
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
    viewInfo.format = m_format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(veDevice.device(), &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }
}

void VeTexture::createCubemapImageView() {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = textureImage;  // Image associated w/ the view.
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = m_format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

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

char *loadTexture(const std::string& filepath, int& width, int& height, int& channels) {
    char *data = (char *)stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    return data;
}

}  // namespace ve