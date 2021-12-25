#pragma once

namespace ve {

class VeGameObject {
   public:
    using id_t = unsigned int;

    static VeGameObject createGameObject() {
        static id_t currentId = 0;
        return VeGameObject{currentId++};
    }

    const id_t getId() { return id; }

   private:
    VeGameObject(id_t objId) : id{objId} {}

    id_t id;
};

}  // namespace ve