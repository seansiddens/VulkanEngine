#include "ve_camera.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

namespace ve {

VeCamera::VeCamera(glm::vec3 position, glm::vec3 target) {
    // Check that the target and position are not the same value.
    assert(glm::dot(target - position, target - position) >
               std::numeric_limits<float>::epsilon() &&
           "Camera target must not be the same as position!");

    setViewTarget(position, target);
}

void VeCamera::setOrthographicProjection(
    float left, float right, float top, float bottom, float near, float far) {
    m_projectionMatrix = glm::mat4{1.0f};
    m_projectionMatrix[0][0] = 2.f / (right - left);
    m_projectionMatrix[1][1] = 2.f / (bottom - top);
    m_projectionMatrix[2][2] = 1.f / (far - near);
    m_projectionMatrix[3][0] = -(right + left) / (right - left);
    m_projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
    m_projectionMatrix[3][2] = -near / (far - near);
}

void VeCamera::setPerspectiveProjection(float fovy, float aspect, float near, float far) {
    assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
    const float tanHalfFovy = std::tan(fovy / 2.f);
    m_projectionMatrix = glm::mat4{0.0f};
    m_projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
    m_projectionMatrix[1][1] = 1.f / (tanHalfFovy);
    m_projectionMatrix[2][2] = far / (far - near);
    m_projectionMatrix[2][3] = 1.f;
    m_projectionMatrix[3][2] = -(far * near) / (far - near);
}

void VeCamera::setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
    assert(glm::dot(direction, direction) > std::numeric_limits<float>::epsilon() &&
           "Direction must be a non-zero vector");

    // Make sure to keep track of member variables when modifying the view matrix.
    m_position = position;
    m_up = up;

    // Construct an orthonormal basis.
    const glm::vec3 w{glm::normalize(direction)};
    const glm::vec3 u{glm::normalize(glm::cross(w, m_up))};
    const glm::vec3 v{glm::cross(w, u)};

    m_viewMatrix = glm::mat4{1.f};
    m_viewMatrix[0][0] = u.x;
    m_viewMatrix[1][0] = u.y;
    m_viewMatrix[2][0] = u.z;
    m_viewMatrix[0][1] = v.x;
    m_viewMatrix[1][1] = v.y;
    m_viewMatrix[2][1] = v.z;
    m_viewMatrix[0][2] = w.x;
    m_viewMatrix[1][2] = w.y;
    m_viewMatrix[2][2] = w.z;
    m_viewMatrix[3][0] = -glm::dot(u, m_position);
    m_viewMatrix[3][1] = -glm::dot(v, m_position);
    m_viewMatrix[3][2] = -glm::dot(w, m_position);
}

void VeCamera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
    setViewDirection(position, target - position, up);
}

// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix.
void VeCamera::setViewYXZ(glm::vec3 position, glm::vec3 orientation) {
    // Update member variables.
    m_position = position;

    // View matrix transforms the camera to the origin and re-orients to face the +Z direction.
    // Move back to origin, then re-orient.
    // View = inv(cam.rotationMat) * -cam.translate
    // Note: The inverse of a rotation matrix is simply it's transpose.
    const float c3 = glm::cos(orientation.z);
    const float s3 = glm::sin(orientation.z);
    const float c2 = glm::cos(orientation.x);
    const float s2 = glm::sin(orientation.x);
    const float c1 = glm::cos(orientation.y);
    const float s1 = glm::sin(orientation.y);
    const glm::vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
    const glm::vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
    const glm::vec3 w{(c2 * s1), (-s2), (c1 * c2)};
    m_viewMatrix = glm::mat4{1.f};
    m_viewMatrix[0][0] = u.x;
    m_viewMatrix[1][0] = u.y;
    m_viewMatrix[2][0] = u.z;
    m_viewMatrix[0][1] = v.x;
    m_viewMatrix[1][1] = v.y;
    m_viewMatrix[2][1] = v.z;
    m_viewMatrix[0][2] = w.x;
    m_viewMatrix[1][2] = w.y;
    m_viewMatrix[2][2] = w.z;
    m_viewMatrix[3][0] = -glm::dot(u, m_position);
    m_viewMatrix[3][1] = -glm::dot(v, m_position);
    m_viewMatrix[3][2] = -glm::dot(w, m_position);
}

}  // namespace ve