#pragma once

#include <glm/ext/matrix_transform.hpp>

#include "ve_camera.hpp"
#include "ve_input.hpp"

namespace ve {

class CameraController {
   public:
    explicit CameraController(VeInput &input) : veInput{input} {}

    virtual void update(VeCamera &cam, float deltaTime) = 0;

   protected:
    VeInput &veInput;
};

// Arcball camera pivots around a central target using the mouse.
class ArcballCam : public CameraController {
   public:
    // Mappings between keybindings and GLFW keycodes.
    struct KeyMappings {
        int zoomIn = GLFW_KEY_LEFT_SHIFT;
        int zoomOut = GLFW_KEY_LEFT_CONTROL;
    };

    explicit ArcballCam(VeInput &input,
               glm::vec3 _target = glm::vec3{0.f, 0.f, 0.f},
               float _zoomSpeed = 3.f);

    void update(VeCamera &cam, float deltaTime) override;

    KeyMappings keys{};
    float zoomSpeed;
    glm::vec3 target;
};

class KeyboardCameraController : public CameraController {
   public:
    struct KeyMappings {
        int moveLeft = GLFW_KEY_A;
        int moveRight = GLFW_KEY_D;
        int moveForward = GLFW_KEY_W;
        int moveBackward = GLFW_KEY_S;
        int moveUp = GLFW_KEY_LEFT_SHIFT;
        int moveDown = GLFW_KEY_LEFT_CONTROL;
        int lookLeft = GLFW_KEY_LEFT;
        int lookRight = GLFW_KEY_RIGHT;
        int lookUp = GLFW_KEY_UP;
        int lookDown = GLFW_KEY_DOWN;
    };

    explicit KeyboardCameraController(VeInput &input, float _moveSpeed = 3.5f, float _lookSpeed = 2.0f);

    void update(VeCamera &cam, float deltaTime) override;

    KeyMappings keys{};
    float moveSpeed;
    float lookSpeed;
};

class MouseCameraController : public CameraController {
   public:
    struct KeyMappings {
        int moveLeft = GLFW_KEY_A;
        int moveRight = GLFW_KEY_D;
        int moveForward = GLFW_KEY_W;
        int moveBackward = GLFW_KEY_S;
        int moveUp = GLFW_KEY_LEFT_SHIFT;
        int moveDown = GLFW_KEY_LEFT_CONTROL;
        int increaseMoveSpeed = GLFW_KEY_E;
        int decreaseMoveSpeed = GLFW_KEY_Q;
    };

    explicit MouseCameraController(VeInput &input, float _moveSpeed = 3.5f, float _lookSpeed = 2.0f);

    void update(VeCamera &cam, float deltaTime) override;

    KeyMappings keys{};
    float moveSpeed;
    float lookSpeed;

   private:
    const float MAX_MOVE_SPEED = 999999.f;
    // Initialize values to face +Z.
    float m_pitch{0.f};
    float m_yaw = glm::pi<float>() / 2.f;

};

}  // namespace ve