#pragma once

#include <glm/gtc/matrix_transform.hpp>

#include "ve_model.hpp"

// std
#include <memory>

namespace ve {

struct TransformComponent {
    glm::vec3 translation{};
    glm::vec3 scale{1.f, 1.f, 1.f};
    glm::vec3 rotation{};

    glm::mat4 mat4();
    glm::mat3 normalMatrix();
};

class VeGameObject {
   public:
    using id_t = unsigned int;

    static VeGameObject createGameObject() {
        static id_t currentId = 0;
        return VeGameObject{currentId++};
    }

    // Remove copy but keep default move constructors and operators.
    VeGameObject(const VeGameObject &) = delete;
    VeGameObject &operator=(const VeGameObject &) = delete;
    VeGameObject(VeGameObject &&) = default;
    VeGameObject &operator=(VeGameObject &&) = default;

    id_t getId() const { return id; }

    std::shared_ptr<VeModel> model{};
    glm::vec3 color{};
    TransformComponent transform{};

   private:
    VeGameObject(id_t objId) : id{objId} {}

    id_t id;
};

}  // namespace ve