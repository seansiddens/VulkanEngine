#pragma once

#include "ve_camera.hpp"
#include "ve_input.hpp"

#include <glm/ext/matrix_transform.hpp>

namespace ve {

class CameraController {
   public:
    CameraController(VeInput &input) : veInput{input} {}

    virtual void update(VeCamera &cam, float deltaTime) = 0;

   protected:
    VeInput &veInput;
};

class ArcballCam : public CameraController {
   public:
    ArcballCam(VeInput &input, glm::vec3 _target = glm::vec3{0.f, 0.f, 0.f}, float _zoomSpeed = 5.f);

    void update(VeCamera &cam, float deltaTime);

   private:
    float zoomSpeed;
    glm::vec3 target;
};

}  // namespace ve