#pragma once

#include <glm/gtc/matrix_transform.hpp>

#include "ve_model.hpp"
#include "ve_texture.hpp"

// std
#include <memory>
#include <unordered_map>

namespace ve {

struct TransformComponent {
    glm::vec3 translation{};
    glm::vec3 scale{1.f, 1.f, 1.f};
    glm::vec3 rotation{};

    [[nodiscard]] glm::mat4 mat4() const;
    [[nodiscard]] glm::mat3 normalMatrix() const;
};

struct Material {
    glm::vec3 albedo{0.5f, 0.f, 0.f};
    float metallic{0.3f};
    float roughness{0.5f};
    float ao{1.f};
};

class VeGameObject {
   public:
    using id_t = unsigned int;
    using Map = std::unordered_map<id_t, VeGameObject>;

    static VeGameObject createGameObject() {
        static id_t currentId = 0;
        return VeGameObject{currentId++};
    }

    // Remove copy but keep default move constructors and operators.
    VeGameObject(const VeGameObject &) = delete;
    VeGameObject &operator=(const VeGameObject &) = delete;
    VeGameObject(VeGameObject &&) = default;
    VeGameObject &operator=(VeGameObject &&) = default;

    [[nodiscard]] id_t getId() const { return id; }

    std::shared_ptr<VeModel> model{};
    glm::vec3 color{};
    TransformComponent transform{};
    std::shared_ptr<VeTexture> texture{};
    Material material{};

   private:
    explicit VeGameObject(id_t objId) : id{objId} {}

    id_t id;
};

}  // namespace ve