#pragma once

#include "Renderer/ve_device.hpp"
#include "Renderer/ve_texture.hpp"

#include <glm/glm.hpp>

namespace ve {

// Struct definition for material parameters which we upload to the device.
struct DeviceMaterial {
    glm::vec3 albedo{1.f, 1.f, 1.f};
    float metallic{1.f};
    float roughness{1.f};
    float ao{1.f};
};

class Material {
   public:
    Material(std::shared_ptr<VeTexture> emptyTexture);
    // Material(VeDevice& device, glm::vec3 albedo, float metallic, float roughness, float ao);

    static std::unique_ptr<Material> createDefaultMaterial(VeDevice &device);

    // Material parameters.
    glm::vec3 m_albedo{1.f, 1.f, 1.f};
    float m_metallic{0.5f};
    float m_roughness{0.5f};
    float m_ao{1.f};

    // Material texture maps.
    std::shared_ptr<VeTexture> m_albedoMap;
    std::shared_ptr<VeTexture> m_metallicMap;
    std::shared_ptr<VeTexture> m_roughnessMap;
    std::shared_ptr<VeTexture> m_aoMap;
};

};  // namespace ve