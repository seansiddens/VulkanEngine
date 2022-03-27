#include "Core/ve_material.hpp"

namespace ve {

Material::Material(std::shared_ptr<VeTexture> emptyTexture) {
    // Set texture maps to empty texture so we use material params by default.
    m_albedoMap = emptyTexture;
    m_metallicMap = emptyTexture;
    m_roughnessMap = emptyTexture;
    m_aoMap = emptyTexture;
}

std::unique_ptr<Material> Material::createDefaultMaterial(VeDevice &device) {
    return std::make_unique<Material>(VeTexture::createEmptyTexture(device));
}

// Material::Material(VeDevice& device, glm::vec3 albedo, float metallic, float roughness, float ao)
//     : m_device{device}, m_albedo{albedo}, m_metallic{metallic}, m_roughness{roughness}, m_ao{ao} {
//     m_emptyTexture = VeTexture::createEmptyTexture(m_device);

//     // Set texture maps to empty texture so we use material params by default.
//     m_albedoMap = m_emptyTexture;
//     m_metallicMap = m_emptyTexture;
//     m_roughnessMap = m_emptyTexture;
//     m_aoMap = m_emptyTexture;
// }


};  // namespace ve