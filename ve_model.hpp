#pragma once

#include "ve_device.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <vector>

namespace ve {

class VeModel {
   public:
    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    VeModel(VeDevice &veDevice, const std::vector<Vertex> &vertices);
    ~VeModel();

    // Delete copy constructors.
    VeModel(const VeModel &) = delete;
    void operator=(const VeModel &) = delete;

    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer);

   private:
    void createVertexBuffers(const std::vector<Vertex> &vertices);

    VeDevice &veDevice;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    uint32_t vertexCount;
};

}  // namespace ve