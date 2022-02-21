#pragma once

#include <memory>

#include "ve_game_object.hpp"
#include "ve_window.hpp"

namespace ve {

class KeyboardMovementController {
   public:
    // Mappings between keybindings and GLFW keycodes.
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

    void moveInPlaneXZ(GLFWwindow *window, float dt, VeGameObject &gameObject);

    KeyMappings keys{};
    float moveSpeed{3.5f};
    float lookSpeed{2.0f};
};

class MouseMovementController {
   public:
    MouseMovementController(GLFWwindow *window, VeGameObject *gameObject);

    // Mappings between keybindings and GLFW keycodes.
    struct KeyMappings {
        int moveLeft = GLFW_KEY_A;
        int moveRight = GLFW_KEY_D;
        int moveForward = GLFW_KEY_W;
        int moveBackward = GLFW_KEY_S;
        int moveUp = GLFW_KEY_E;
        int moveDown = GLFW_KEY_Q;
    };

    void update(GLFWwindow *window, float dt, VeGameObject &gameObject);
    void updateObject(float xOffset, float yOffset);

    KeyMappings keys{};
    float moveSpeed{3.5f};
    float lookSpeed{0.01f};
    double lastX, lastY;
    float pitch{0.f};
    float yaw{90.f};
    bool firstMouse{false};

    VeGameObject *gameObject;
};

}  // namespace ve