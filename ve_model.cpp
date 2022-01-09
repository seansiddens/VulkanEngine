#include "ve_model.hpp"

#include "ve_utils.hpp"

// libs
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

// std
#include <cassert>
#include <cstring>
#include <iostream>
#include <unordered_map>

namespace std {
template <>
struct hash<ve::VeModel::Vertex> {
    size_t operator()(ve::VeModel::Vertex const &vertex) const {
        size_t seed = 0;
        ve::hashCombine(seed, vertex.position, vertex.color, vertex.uv);
        return seed;
    }
};

}  // namespace std

namespace ve {

VeModel::VeModel(VeDevice &veDevice, const VeModel::Builder &builder) : veDevice{veDevice} {
    createVertexBuffers(builder.vertices);
    createIndexBuffers(builder.indices);
}

VeModel::~VeModel() {
    // Clean up vertex buffer memory.
    vkDestroyBuffer(veDevice.device(), vertexBuffer, nullptr);
    vkFreeMemory(veDevice.device(), vertexBufferMemory, nullptr);

    if (hasIndexBuffer) {
        vkDestroyBuffer(veDevice.device(), indexBuffer, nullptr);
        vkFreeMemory(veDevice.device(), indexBufferMemory, nullptr);
    }
}

void VeModel::createVertexBuffers(const std::vector<Vertex> &vertices) {
    vertexCount = static_cast<uint32_t>(vertices.size());
    // Check that odel contains at least one triangle.
    assert(vertexCount >= 3 && "Vertex count must be at least 3!");

    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

    // Create the staging buffer.
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    veDevice.createBuffer(
        bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
        stagingBufferMemory);

    // Create a region of host memory that is directly mapped to device memory.
    void *data;
    vkMapMemory(veDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
    // Copy vertex data to the host memory-mapped staging buffer. Since
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT is enabled, this data is automatically flushed to device
    // memory.
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(veDevice.device(), stagingBufferMemory);

    // Vertex buffer uses optimal device local memory.
    veDevice.createBuffer(bufferSize,
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

    // Copy vertex data from the staging buffer to the vertex buffer.
    veDevice.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    // Clean-up the staging buffer since it is no longer needed.
    vkDestroyBuffer(veDevice.device(), stagingBuffer, nullptr);
    vkFreeMemory(veDevice.device(), stagingBufferMemory, nullptr);
}

void VeModel::createIndexBuffers(const std::vector<uint32_t> &indices) {
    // Determine if we have a valid index buffer.
    indexCount = static_cast<uint32_t>(indices.size());
    hasIndexBuffer = indexCount > 0;
    if (!hasIndexBuffer) {
        return;
    }

    VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;

    // Create the staging buffer.
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    veDevice.createBuffer(
        bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
        stagingBufferMemory);

    // Create a region of host memory that is directly mapped to device memory.
    void *data;
    vkMapMemory(veDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
    // Copy indice data to the host memory-mapped staging buffer. Since
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT is enabled, this data is automatically flushed to device
    // memory.
    memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(veDevice.device(), stagingBufferMemory);

    // Index buffer uses optimal device local memory.
    veDevice.createBuffer(bufferSize,
                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    // Copy indice data from the staging buffer to the index buffer.
    veDevice.copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    // Clean-up the staging buffer since it is no longer needed.
    vkDestroyBuffer(veDevice.device(), stagingBuffer, nullptr);
    vkFreeMemory(veDevice.device(), stagingBufferMemory, nullptr);
}

std::unique_ptr<VeModel> VeModel::createModelFromFile(VeDevice &device,
                                                      const std::string &filepath) {
    Builder builder{};
    builder.loadModel(filepath);

    std::cout << "Size: " << builder.vertices.size() << '\n';

    return std::make_unique<VeModel>(device, builder);
}

void VeModel::draw(VkCommandBuffer commandBuffer) {
    if (hasIndexBuffer) {
        vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
    } else {
        vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
    }
}

void VeModel::bind(VkCommandBuffer commandBuffer) {
    VkBuffer buffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

    if (hasIndexBuffer) {
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }
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
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[0].offset = offsetof(Vertex, position);
    // Color
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[1].offset = offsetof(Vertex, color);
    return attributeDescriptions;
}

void VeModel::Builder::loadModel(const std::string &filepath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
        // Loading model failed.
        throw std::runtime_error(warn + err);
    }

    vertices.clear();
    indices.clear();

    // Keep track of which vertices have already been seen and keep track of the original position
    // where it was added.
    std::unordered_map<Vertex, uint32_t> uniqueVertices;

    // Loop through each indices of each face element of the model.
    for (const auto &shape : shapes) {
        for (const auto &index : shape.mesh.indices) {
            // Initialize vertex w/ model vertex info.
            Vertex vertex{};
            if (index.vertex_index >= 0) {
                vertex.position = {attrib.vertices[3 * index.vertex_index + 0],
                                   attrib.vertices[3 * index.vertex_index + 1],
                                   attrib.vertices[3 * index.vertex_index + 2]};
            }

            auto colorIndex = 3 * index.vertex_index + 2;
            if (colorIndex < attrib.colors.size()) {
                vertex.color = {attrib.colors[colorIndex - 2], attrib.colors[colorIndex - 1],
                                attrib.colors[colorIndex - 0]};
            } else {
                vertex.color = {1.f, 1.f, 1.f};  // Set default color.
            }

            if (index.normal_index >= 0) {
                vertex.normal = {attrib.normals[3 * index.normal_index + 0],
                                 attrib.normals[3 * index.normal_index + 1],
                                 attrib.normals[3 * index.normal_index + 2]};
            }

            if (index.texcoord_index >= 0) {
                vertex.uv = {attrib.texcoords[2 * index.texcoord_index + 0],
                             attrib.texcoords[2 * index.texcoord_index + 1]};
            }

            // Add to our vertex list.
            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            // Add vertex index to indices vector.
            indices.push_back(uniqueVertices[vertex]);
        }
    }
}

}  // namespace ve