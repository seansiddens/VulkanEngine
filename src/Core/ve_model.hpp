#pragma once

#include "Renderer/ve_buffer.hpp"
#include "Renderer/ve_device.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <memory>
#include <vector>

namespace ve {

class VeModel {
   public:
    struct Vertex {
        glm::vec3 position{};
        glm::vec3 color{};
        glm::vec3 normal{};
        glm::vec2 uv{};

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

        // Overload the equality operator.
        bool operator==(const Vertex &other) const {
            return position == other.position && color == other.color && normal == other.normal &&
                   uv == other.uv;
        }
    };

    struct Builder {
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};

        void loadModel(const std::string &filepath);
    };

    VeModel(VeDevice &veDevice, const VeModel::Builder &buider);
    ~VeModel();

    // Delete copy constructors.
    VeModel(const VeModel &) = delete;
    void operator=(const VeModel &) = delete;

    static std::unique_ptr<VeModel> createModelFromFile(VeDevice &device,
                                                        const std::string &filepath);

    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer);

    // TODO: This should not be public, just a temp fix.
    VeDevice &veDevice;

   private:
    void createVertexBuffers(const std::vector<Vertex> &vertices);
    void createIndexBuffers(const std::vector<uint32_t> &indices);


    std::unique_ptr<VeBuffer> vertexBuffer;
    uint32_t vertexCount;

    bool hasIndexBuffer{false};
    std::unique_ptr<VeBuffer> indexBuffer;
    uint32_t indexCount;
};

}  // namespace ve