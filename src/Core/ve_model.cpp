#include "Core/ve_model.hpp"

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

// Pathing is done from the build directory, so we define a macro to orient us automatically
// in the project root directory.
#ifndef ENGINE_DIR
#define ENGINE_DIR ""
#endif

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

VeModel::~VeModel() {}

void VeModel::createVertexBuffers(const std::vector<Vertex> &vertices) {
    vertexCount = static_cast<uint32_t>(vertices.size());
    // Check that model contains at least one triangle.
    assert(vertexCount >= 3 && "Vertex count must be at least 3!");

    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
    uint32_t vertexSize = sizeof(vertices[0]);

    // Create the staging buffer.
    VeBuffer stagingBuffer{
        veDevice,
        vertexSize,
        vertexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    // Write vertex data to the staging buffer.
    stagingBuffer.map();
    stagingBuffer.writeToBuffer((void *)vertices.data());

    // Create the vertex buffer.
    vertexBuffer = std::make_unique<VeBuffer>(
        veDevice,
        vertexSize,
        vertexCount,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    veDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
}

void VeModel::createIndexBuffers(const std::vector<uint32_t> &indices) {
    // Determine if we have a valid index buffer.
    indexCount = static_cast<uint32_t>(indices.size());
    hasIndexBuffer = indexCount > 0;
    if (!hasIndexBuffer) {
        return;
    }

    VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
    uint32_t indexSize = sizeof(indices[0]);

    // Create the staging buffer.
    VeBuffer stagingBuffer{
        veDevice,
        indexSize,
        indexCount,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

    // Write vertex data to the staging buffer.
    stagingBuffer.map();  // Unmapped when destructor is called.
    stagingBuffer.writeToBuffer((void *)indices.data());

    // Create the vertex buffer.
    indexBuffer = std::make_unique<VeBuffer>(
        veDevice,
        indexSize,
        indexCount,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    veDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
}

std::unique_ptr<VeModel> VeModel::createModelFromFile(VeDevice &device,
                                                      const std::string &filepath) {
    Builder builder{};
    builder.loadModel(filepath);

    // std::cout << "Model size: " << builder.vertices.size() << '\n';

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
    VkBuffer buffers[] = {vertexBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

    if (hasIndexBuffer) {
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
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
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
    attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
    attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
    attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});

    return attributeDescriptions;
}

void VeModel::Builder::loadModel(const std::string &filepath) {
    std::string enginePath = ENGINE_DIR + filepath;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, enginePath.c_str())) {
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

                vertex.color = {attrib.colors[3 * index.vertex_index + 0],
                                attrib.colors[3 * index.vertex_index + 1],
                                attrib.colors[3 * index.vertex_index + 2]};
            }

            if (index.normal_index >= 0) {
                vertex.normal = {attrib.normals[3 * index.normal_index + 0],
                                 attrib.normals[3 * index.normal_index + 1],
                                 attrib.normals[3 * index.normal_index + 2]};
            }

            if (index.texcoord_index >= 0) {
                vertex.uv = {attrib.texcoords[2 * index.texcoord_index + 0],
                             1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
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
    std::cout << "Vertice count: " << vertices.size() << "\n";
}

}  // namespace ve