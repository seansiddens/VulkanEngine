#pragma once

#include <glm/ext/matrix_transform.hpp>

#include "ve_camera.hpp"
#include "ve_input.hpp"

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
    // Mappings between keybindings and GLFW keycodes.
    struct KeyMappings {
        int zoomIn = GLFW_KEY_LEFT_SHIFT;
        int zoomOut = GLFW_KEY_LEFT_CONTROL;
    };

    ArcballCam(VeInput &input,
               glm::vec3 _target = glm::vec3{0.f, 0.f, 0.f},
               float _zoomSpeed = 3.f);

    void update(VeCamera &cam, float deltaTime);

    KeyMappings keys{};
    float zoomSpeed;
    glm::vec3 target;
};

}  // namespace ve