#include "ve_camera.hpp"

#include <cassert>
#include <glm/ext/matrix_transform.hpp>
#include <iostream>
#include <limits>

namespace ve {

VeCamera::VeCamera(VeInput &input, glm::vec3 _position, glm::vec3 _pivot) : veInput{input} {
    assert(glm::dot(_pivot - _position, _pivot - _position) > std::numeric_limits<float>::epsilon() &&
           "Pivot must not be the same as position!");
    position = _position;
    pivot = _pivot;
    setViewTarget(position, pivot);
}

void VeCamera::setOrthographicProjection(
    float left, float right, float top, float bottom, float near, float far) {
    projectionMatrix = glm::mat4{1.0f};
    projectionMatrix[0][0] = 2.f / (right - left);
    projectionMatrix[1][1] = 2.f / (bottom - top);
    projectionMatrix[2][2] = 1.f / (far - near);
    projectionMatrix[3][0] = -(right + left) / (right - left);
    projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
    projectionMatrix[3][2] = -near / (far - near);
}

void VeCamera::setPerspectiveProjection(float fovy, float aspect, float near, float far) {
    assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
    const float tanHalfFovy = tan(fovy / 2.f);
    projectionMatrix = glm::mat4{0.0f};
    projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
    projectionMatrix[1][1] = 1.f / (tanHalfFovy);
    projectionMatrix[2][2] = far / (far - near);
    projectionMatrix[2][3] = 1.f;
    projectionMatrix[3][2] = -(far * near) / (far - near);
}

void VeCamera::setViewDirection(glm::vec3 _position, glm::vec3 direction, glm::vec3 up) {
    assert(glm::dot(direction, direction) > std::numeric_limits<float>::epsilon() &&
           "Direction must be a non-zero vector");
    position = _position;

    // Construct an orthonormal basis.
    const glm::vec3 w{glm::normalize(direction)};
    const glm::vec3 u{glm::normalize(glm::cross(w, up))};
    const glm::vec3 v{glm::cross(w, u)};

    viewMatrix = glm::mat4{1.f};
    viewMatrix[0][0] = u.x;
    viewMatrix[1][0] = u.y;
    viewMatrix[2][0] = u.z;
    viewMatrix[0][1] = v.x;
    viewMatrix[1][1] = v.y;
    viewMatrix[2][1] = v.z;
    viewMatrix[0][2] = w.x;
    viewMatrix[1][2] = w.y;
    viewMatrix[2][2] = w.z;
    viewMatrix[3][0] = -glm::dot(u, position);
    viewMatrix[3][1] = -glm::dot(v, position);
    viewMatrix[3][2] = -glm::dot(w, position);
}

void VeCamera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
    setViewDirection(position, target - position, up);
}

void VeCamera::setViewYXZ(glm::vec3 position, glm::vec3 rotation) {
    // View matrix transforms the camera to the origin and re-orients to face the +Z direction.
    // Move back to origin, then re-orient.
    // View = inv(cam.rotationMat) * -cam.translate
    // Note: The inverse of a rotation matrix is simply it's transpose.
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);
    const glm::vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
    const glm::vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
    const glm::vec3 w{(c2 * s1), (-s2), (c1 * c2)};
    viewMatrix = glm::mat4{1.f};
    viewMatrix[0][0] = u.x;
    viewMatrix[1][0] = u.y;
    viewMatrix[2][0] = u.z;
    viewMatrix[0][1] = v.x;
    viewMatrix[1][1] = v.y;
    viewMatrix[2][1] = v.z;
    viewMatrix[0][2] = w.x;
    viewMatrix[1][2] = w.y;
    viewMatrix[2][2] = w.z;
    viewMatrix[3][0] = -glm::dot(u, position);
    viewMatrix[3][1] = -glm::dot(v, position);
    viewMatrix[3][2] = -glm::dot(w, position);
}

void VeCamera::update(float deltaTime) {
    // A movement from left to right = 2 * PI = 360 deg
    float angleScaleX = (2.f * M_PI / static_cast<float>(veInput.getWindow().getExtent().width));
    // A movement from top to bottom = PI = 180 deg.
    float angleScaleY = (M_PI / static_cast<float>(veInput.getWindow().getExtent().height));

    forwardDir = glm::normalize(pivot - position);

    // Zoom in/out.
    glm::vec3 newPosition = position;
    if (veInput.getKey(GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        newPosition += (zoomSpeed * deltaTime * forwardDir);
        // Check that newPosition != target.
        glm::vec3 dstFromPivot = newPosition - pivot;
        if (glm::dot(dstFromPivot, dstFromPivot) < std::numeric_limits<float>::epsilon()) {
            newPosition = position; // Undo translation.
        }
    }

    if (veInput.getKey(GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        newPosition -= (zoomSpeed * deltaTime * forwardDir);
    }

    // Amount to rotate.
    float deltaAngleX = (veInput.getDeltaX() * -1.0) * angleScaleX;
    float deltaAngleY = (veInput.getDeltaY() * -1.0) * angleScaleY;

    if (veInput.getMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        // Rotate camera object around pivot point about the Y axis.
        glm::mat4 rotMatrixX(1.f);
        rotMatrixX = glm::rotate(rotMatrixX, deltaAngleX, glm::vec3{0.f, -1.f, 0.f});
        newPosition = rotMatrixX * glm::vec4(newPosition - pivot, 1.f) + glm::vec4(pivot, 1.f);

        // Rotate camera around pivot about the camera object's right dir.
        glm::mat4 rotationMatrixY(1.0f);
        rotationMatrixY = glm::rotate(rotationMatrixY, deltaAngleY, getRightDir());
        newPosition = rotationMatrixY * glm::vec4(newPosition - pivot, 1.f) + glm::vec4(pivot, 1.f);

    }

    setViewTarget(newPosition, pivot);
}

}  // namespace ve