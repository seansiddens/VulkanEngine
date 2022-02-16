#include "camera_controller.hpp"

#include <cmath>
#include <glm/ext/matrix_transform.hpp>
#include <iostream>

namespace ve {

ArcballCam::ArcballCam(VeInput& input, glm::vec3 _target, float _zoomSpeed)
    : CameraController(input), zoomSpeed{_zoomSpeed}, target{_target} {}

void ArcballCam::update(VeCamera& cam, float deltaTime) {
    // A movement from left to right = 2 * PI = 360 deg
    auto angleScaleX = static_cast<float>(2.f * M_PI / veInput.getWindow().getExtent().width);
    // A movement from top to bottom = PI = 180 deg.
    auto angleScaleY = static_cast<float>(M_PI / veInput.getWindow().getExtent().height);

    glm::vec3 forwardDir = glm::normalize(target - cam.getPosition());

    // Zoom in/out.
    glm::vec3 newPosition = cam.getPosition();
    if (veInput.getKey(keys.zoomIn)) {
        newPosition += (zoomSpeed * deltaTime * forwardDir);
        // Check that newPosition != target.
        glm::vec3 dstFromPivot = newPosition - target;
        if (glm::dot(dstFromPivot, dstFromPivot) < std::numeric_limits<float>::epsilon()) {
            newPosition = cam.getPosition();  // Undo translation.
        }
    }

    if (veInput.getKey(keys.zoomOut)) {
        newPosition -= (zoomSpeed * deltaTime * forwardDir);
    }

    // Amount to rotate.
    auto deltaAngleX = static_cast<float>((veInput.getDeltaX() * -1.0) * angleScaleX);
    auto deltaAngleY = static_cast<float>((veInput.getDeltaY() * -1.0) * angleScaleY);

    if (veInput.getMouseButton(GLFW_MOUSE_BUTTON_LEFT)) {
        // Rotate camera object around pivot point about the Y axis.
        glm::mat4 rotMatrixX(1.f);
        rotMatrixX = glm::rotate(rotMatrixX, deltaAngleX, glm::vec3{0.f, -1.f, 0.f});
        newPosition = rotMatrixX * glm::vec4(newPosition - target, 1.f) + glm::vec4(target, 1.f);

        // Rotate camera around pivot about the camera object's right dir.
        glm::mat4 rotationMatrixY(1.0f);
        rotationMatrixY = glm::rotate(rotationMatrixY, deltaAngleY, cam.getRightDir());
        newPosition =
            rotationMatrixY * glm::vec4(newPosition - target, 1.f) + glm::vec4(target, 1.f);
    }

    cam.setViewTarget(newPosition, target);
}

KeyboardCameraController::KeyboardCameraController(VeInput& input,
                                                   float _moveSpeed,
                                                   float _lookSpeed)
    : CameraController(input), moveSpeed{_moveSpeed}, lookSpeed{_lookSpeed} {}

void KeyboardCameraController::update(VeCamera& cam, float deltaTime) {
    // TODO: Implement this.
    //    glm::vec3 rotate{0.f};
    //    if (veInput.getKey(keys.lookRight) == GLFW_PRESS) rotate.y += 1.f;
    //    if (veInput.getKey(keys.lookLeft) == GLFW_PRESS) rotate.y -= 1.f;
    //    if (veInput.getKey(keys.lookUp) == GLFW_PRESS) rotate.x += 1.f;
    //    if (veInput.getKey(keys.lookDown) == GLFW_PRESS) rotate.x -= 1.f;
    //
    //    // Only update if the rotate vector is non-zero.
    //    if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
    //        cam.transform.rotation += lookSpeed * dt * glm::normalize(rotate);
    //    }
    //
    //    // Limit pitch value between about +/- 85 degrees.
    //    gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x,
    //    -1.5f, 1.5f);
    //    gameObject.transform.rotation.y =
    //        glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());
    //
    //    float yaw = gameObject.transform.rotation.y;
    //    const glm::vec3 forwardDir{sin(yaw), 0.f, cos(yaw)};
    //    const glm::vec3 rightDir{forwardDir.z, 0.f, -forwardDir.x};
    //    const glm::vec3 upDir{0.f, -1.f, 0.f};
    //
    //    glm::vec3 moveDir{0.f};
    //    if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
    //    if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
    //    if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
    //    if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
    //    if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
    //    if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;
    //
    //    // Only update if the move vector is non-zero.
    //    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
    //        gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
    //    }
}

MouseCameraController::MouseCameraController(VeInput& input, float _moveSpeed, float _lookSpeed)
    : CameraController(input), moveSpeed{_moveSpeed}, lookSpeed{_lookSpeed} {
    glfwSetInputMode(input.getWindow().getGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void MouseCameraController::update(VeCamera& cam, float deltaTime) {
    m_yaw -= veInput.getDeltaX() * deltaTime * lookSpeed;
    m_pitch += veInput.getDeltaY() * deltaTime * lookSpeed;

    // Clamp pitch between +/- 85 degrees.
    m_pitch = glm::clamp(m_pitch, -1.5f, 1.5f);

    // Calulate the new direction vector based off of pitch and yaw.
    glm::vec3 direction{};
    direction.x = std::cos(m_yaw) * std::cos(m_pitch);
    direction.y = std::sin(m_pitch);
    direction.z = std::sin(m_yaw) * std::cos(m_pitch);

    // Update camera w/ new direction.
    cam.setViewDirection(cam.getPosition(), direction);

    glm::vec3 camPos = cam.getPosition();
    // Move camera.
    if (veInput.getKey(keys.moveForward)) {
        camPos += cam.getViewDir() * moveSpeed * deltaTime;
    }
    if (veInput.getKey(keys.moveBackward)) {
        camPos -= cam.getViewDir() * moveSpeed * deltaTime;
    }
    if (veInput.getKey(keys.moveLeft)) {
        camPos -= cam.getRightDir() * moveSpeed * deltaTime;
    }
    if (veInput.getKey(keys.moveRight)) {
        camPos += cam.getRightDir() * moveSpeed * deltaTime;
    }
    if (veInput.getKey(keys.moveUp)) {
        camPos += cam.getUpDir() * moveSpeed * deltaTime;
    }
    if (veInput.getKey(keys.moveDown)) {
        camPos -= cam.getUpDir() * moveSpeed * deltaTime;
    }

    // Update cam w/ new position.
    cam.setViewDirection(camPos, direction);
}

}  // namespace ve