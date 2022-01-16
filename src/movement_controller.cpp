#include "movement_controller.hpp"

#include <iostream>

namespace ve {

void KeyboardMovementController::moveInPlaneXZ(GLFWwindow *window, float dt,
                                               VeGameObject &gameObject) {
    glm::vec3 rotate{0.f};
    if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotate.y += 1.f;
    if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotate.y -= 1.f;
    if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotate.x += 1.f;
    if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotate.x -= 1.f;

    // Only update if the rotate vector is non-zero.
    if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
        gameObject.transform.rotation += lookSpeed * dt * glm::normalize(rotate);
    }

    // Limit pitch value between about +/- 85 degrees.
    gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
    gameObject.transform.rotation.y =
        glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());

    float yaw = gameObject.transform.rotation.y;
    const glm::vec3 forwardDir{sin(yaw), 0.f, cos(yaw)};
    const glm::vec3 rightDir{forwardDir.z, 0.f, -forwardDir.x};
    const glm::vec3 upDir{0.f, -1.f, 0.f};

    glm::vec3 moveDir{0.f};
    if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
    if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
    if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
    if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
    if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
    if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;

    // Only update if the move vector is non-zero.
    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
        gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
    }
}

MouseMovementController::MouseMovementController(GLFWwindow *window, VeGameObject *gameObject)
    : gameObject{gameObject} {
    // Have the window hide the cursor and capture it.
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the initial values for the cursor's pos in previous frame.
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    // Set to center of window.
    lastX = static_cast<double>(width) / 2.0;
    lastY = static_cast<double>(height) / 2.0;

    // Set the GLFWwindow user pointer to point to this instance.
    glfwSetWindowUserPointer(window, static_cast<void *>(this));
}

void MouseMovementController::update(GLFWwindow *window, float dt, VeGameObject &gameObject) {
    // The current position of the cursor.
    double currentX, currentY;
    glfwGetCursorPos(window, &currentX, &currentY);

    // Get the offset of the cursor between the last frame and the current.
    double deltaX = currentX - lastX;
    double deltaY = lastY - currentY;
    // Set last cursor pos for next frame.
    lastX = currentX;
    lastY = currentY;

    // Update rotation.
    glm::vec3 rotate{0.f};
    // if (deltaX > std::numeric_limits<double>::epsilon()) {
    //     rotate.y += deltaX;
    // } else if (deltaX * -1.0 > std::numeric_limits<double>::epsilon()) {
    //     rotate.y -= deltaX;
    // }
    // if (deltaY > std::numeric_limits<double>::epsilon()) {
    //     rotate.x += deltaY;
    // } else if (deltaY * -1 > std::numeric_limits<double>::epsilon()) {
    //     rotate.x -= deltaY;
    // }
    rotate.y += deltaX;
    rotate.x += deltaY;

    // Debug stuff
    std::cout << "Delta time: " << dt << '\n';
    std::cout << rotate.x << ' ' << rotate.y << ' ' << rotate.z << '\n';
    std::cout << "Delta X: " << deltaX << ", delta Y: " << deltaY << '\n';

    // Only update if the rotate vector is non-zero.
    if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
        gameObject.transform.rotation += dt * lookSpeed * glm::normalize(rotate);
    }

    // Limit pitch value between about +/- 85 degrees.
    gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
    gameObject.transform.rotation.y =
        glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());

    // Get orientation of the game object.
    float yaw = gameObject.transform.rotation.y;
    const glm::vec3 forwardDir{sin(yaw), 0.f, cos(yaw)};
    const glm::vec3 rightDir{forwardDir.z, 0.f, -forwardDir.x};
    const glm::vec3 upDir{0.f, -1.f, 0.f};

    // Update translation.
    glm::vec3 moveDir{0.f};
    if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
    if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
    if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
    if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
    if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
    if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;

    // Only update if the move vector is non-zero.
    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
        gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
    }
}

void MouseMovementController::updateObject(float xOffset, float yOffset) {
    std::cout << "Cursor offset: (" << xOffset << ", " << yOffset << ")\n\n";

    xOffset *= lookSpeed;
    yOffset *= lookSpeed;

    yaw += xOffset;
    pitch += yOffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 direction{0.f};
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    // direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    // Only update if the rotate vector is non-zero.
    if (glm::dot(direction, direction) > std::numeric_limits<float>::epsilon()) {
        gameObject->transform.rotation = glm::normalize(direction);
    }
}

}  // namespace ve
