#pragma once

#include "ve_model.hpp"

// std
#include <memory>

namespace ve {

struct Transform2dComponent {
    glm::vec2 translation{};
    glm::vec2 scale{1.f, 1.f};
    float rotation;

    glm::mat2 mat2() {
        const float s = glm::sin(rotation);
        const float c = glm::cos(rotation);
        glm::mat2 rotMatrix{{c, s}, {-s, c}};

        glm::mat2 scaleMat{{scale.x, 0.f}, {0.f, scale.y}};
        return rotMatrix * scaleMat;
    }
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
    Transform2dComponent transform2d{};

   private:
    VeGameObject(id_t objId) : id{objId} {}

    id_t id;
};

}  // namespace ve