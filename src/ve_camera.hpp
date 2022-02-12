#pragma once

#include "ve_window.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <memory>

namespace ve {

class VeCamera {
   public:
    VeCamera(glm::vec3 position = glm::vec3{0.f}, glm::vec3 target = glm::vec3{0.f, 1.f, 0.f});

    void setOrthographicProjection(
        float left, float right, float top, float bottom, float near, float far);
    void setPerspectiveProjection(float vfovy, float aspect, float near, float far);

    void setViewDirection(glm::vec3 position,
                          glm::vec3 direction,
                          glm::vec3 up = glm::vec3{0.f, -1.f, 0.f});
    void setViewTarget(glm::vec3 position,
                       glm::vec3 target,
                       glm::vec3 up = glm::vec3{0.f, -1.f, 0.f});
    // Use Euler's angles to orient the camera.
    void setViewYXZ(glm::vec3 position, glm::vec3 rotation);
    void setPosition(glm::vec3 _position) { position = _position; }

    glm::vec3 getViewDir() const { return glm::transpose(viewMatrix)[2]; }
    glm::vec3 getRightDir() const { return glm::transpose(viewMatrix)[0]; }
    glm::vec3 getPosition() const { return position; }

    const glm::mat4 &getProjection() const { return projectionMatrix; }
    const glm::mat4 &getView() const { return viewMatrix; }


   private:
    glm::mat4 projectionMatrix{1.f};
    glm::mat4 viewMatrix{1.f};

    glm::vec3 up{0.f, -1.f, 0.f};
    glm::vec3 position{0.f};
};

}  // namespace ve