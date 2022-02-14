#include "camera_controller.hpp"

#include <iostream>

namespace ve {

ArcballCam::ArcballCam(VeInput& input, glm::vec3 _target, float _zoomSpeed)
    : CameraController(input), zoomSpeed{_zoomSpeed}, target{_target} {}

void ArcballCam::update(VeCamera& cam, float deltaTime) {
    // A movement from left to right = 2 * PI = 360 deg
    float angleScaleX = (2.f * M_PI / static_cast<float>(veInput.getWindow().getExtent().width));
    // A movement from top to bottom = PI = 180 deg.
    float angleScaleY = (M_PI / static_cast<float>(veInput.getWindow().getExtent().height));

    glm::vec3 forwardDir = glm::normalize(target - cam.getPosition());

    // Zoom in/out.
    glm::vec3 newPosition = cam.getPosition();
    if (veInput.getKey(keys.zoomIn) == GLFW_PRESS) {
        newPosition += (zoomSpeed * deltaTime * forwardDir);
        // Check that newPosition != target.
        glm::vec3 dstFromPivot = newPosition - target;
        if (glm::dot(dstFromPivot, dstFromPivot) < std::numeric_limits<float>::epsilon()) {
            newPosition = cam.getPosition();  // Undo translation.
        }
    }

    if (veInput.getKey(keys.zoomOut) == GLFW_PRESS) {
        newPosition -= (zoomSpeed * deltaTime * forwardDir);
    }

    // Amount to rotate.
    float deltaAngleX = (veInput.getDeltaX() * -1.0) * angleScaleX;
    float deltaAngleY = (veInput.getDeltaY() * -1.0) * angleScaleY;

    if (veInput.getMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
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
                                                   float _moveSpeed = 3.5f,
                                                   float _lookSpeed = 2.0f)
    : CameraController(input), moveSpeed{_moveSpeed}, lookSpeed{_lookSpeed} {

    }

void update(VeCamera& cam, float deltaTime) {
    glm::vec3 rotate{0.f};
    if (veInput.getKey(window, keys.lookRight) == GLFW_PRESS) rotate.y += 1.f;
    if (veInput.getKey(window, keys.lookLeft) == GLFW_PRESS) rotate.y -= 1.f;
    if (veInput.getKey(window, keys.lookUp) == GLFW_PRESS) rotate.x += 1.f;
    if (veInput.getKey(window, keys.lookDown) == GLFW_PRESS) rotate.x -= 1.f;

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

}  // namespace ve