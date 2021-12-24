#include "ve_model.hpp"

// std
#include <cassert>
#include <cstring>

namespace ve {

VeModel::VeModel(VeDevice &veDevice, const std::vector<Vertex> &vertices) : veDevice{veDevice} {
    createVertexBuffers(vertices);
}

VeModel::~VeModel() {
    vkDestroyBuffer(veDevice.device(), vertexBuffer, nullptr);
    vkFreeMemory(veDevice.device(), vertexBufferMemory, nullptr);
}

void VeModel::createVertexBuffers(const std::vector<Vertex> &vertices) {
    vertexCount = static_cast<uint32_t>(vertices.size());
    // Check that odel contains at least one triangle.
    assert(vertexCount >= 3 && "Vertex count must be at least 3!");

    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
    // Allocate buffer.
    veDevice.createBuffer(
        bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer,
        vertexBufferMemory);

    // Create a region of host memory that is directly mapped to device memory.
    void *data;
    vkMapMemory(veDevice.device(), vertexBufferMemory, 0, bufferSize, 0, &data);
    // Copy vertex data to the host memory-mapped region. Since VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    // is enabled, this data is automatically flushed to device memory.
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(veDevice.device(), vertexBufferMemory);
}

void VeModel::draw(VkCommandBuffer commandBuffer) {
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
}

void VeModel::bind(VkCommandBuffer commandBuffer) {
    VkBuffer buffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
}

std::vector<VkVertexInputBindingDescription> VeModel::Vertex::getBindingDescriptions() {
    std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> VeModel::Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
    // Both attributes have the same binding since we are interleaving them in the same buffer.
    // Position
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;  // vec2
    attributeDescriptions[0].offset = offsetof(Vertex, position);
    // Color
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[1].offset = offsetof(Vertex, color);
    return attributeDescriptions;
}

}  // namespace ve