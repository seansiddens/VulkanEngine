#pragma once

#include "ve_model.hpp"

// std
#include <memory>

namespace ve {

struct Transform2dComponent {
    glm::vec2 translation{};

    glm::mat2 mat2() { return glm::mat2{1.f}; }
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