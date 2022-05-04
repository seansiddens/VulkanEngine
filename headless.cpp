#include "headless.hpp"

#include "Core/camera_controller.hpp"
#include "Core/movement_controller.hpp"
#include "Core/ve_camera.hpp"
#include "Core/ve_frame_info.hpp"
#include "Core/ve_material.hpp"
#include "Renderer/ve_texture.hpp"
#include "systems/point_light_system.hpp"
#include "systems/simple_render_system.hpp"
#include "systems/skybox_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace ve {

struct GlobalUbo {
    glm::mat4 projection{1.f};
    glm::mat4 view{1.f};
    glm::vec3 lightPosition{0.f, -4.f, 0.0};
    alignas(16) glm::vec3 lightColor{150.f, 150.f, 150.f};
    alignas(16) glm::vec3 viewPos;
};

Headless::Headless() {
}

void Headless::run() {
    // Set clear color.
    veRenderer.setClearColor({0.05, 0.05, 0.05, 1.f});

    // Start game loop.
    while (!veWindow.shouldClose()) {
        // beginFrame() will return a nullptr if swap chain needs to be recreated (window resized).
        if (auto commandBuffer = veRenderer.beginFrame()) {
            int frameIndex = veRenderer.getFrameIndex();
            FrameInfo frameInfo{frameIndex,
                                frameTime,
                                commandBuffer,
                                camera,
                                globalDescriptorSets[frameIndex],
                                gameObjects};

            // update
            //  Set up ubo
            GlobalUbo ubo{};
            ubo.projection = camera.getProjection();
            ubo.view = camera.getView();
            ubo.viewPos = camera.getPosition();
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            // Begin render pass.
            veRenderer.beginSwapChainRenderPass(commandBuffer);

            simpleRenderSystem.renderGameObjects(frameInfo);
            pointLightSystem.render(frameInfo);
            skyboxSystem.renderSkybox(frameInfo);

            // Render ImGui.
            VeImGui::render(commandBuffer);

            // End render pass and frame.
            veRenderer.endSwapChainRenderPass(commandBuffer);
            veRenderer.endFrame();
        }
    }

    // Wait for GPU to finish before exiting.
    vkDeviceWaitIdle(veDevice.device());

    frame += 1;
}

void FirstApp::initScene() { 
    loadAssets();
    loadTestScene(); 
}

void FirstApp::loadAssets() {
    // Load textures.
    // m_textures["rustedIronAlbedo"] = VeTexture::createTextureFromFile(
    //     veDevice, "assets/materials/rusted_iron2/rustediron2_albedo.png");
    // m_textures["rustedIronMetallic"] = VeTexture::createTextureFromFile(
    //     veDevice, "assets/materials/rusted_iron2/rustediron2_metallic.png");
    // m_textures["rustedIronRoughness"] = VeTexture::createTextureFromFile(
    //     veDevice, "assets/materials/rusted_iron2/rustediron2_roughness.png");
    m_textures["empty"] = VeTexture::createEmptyTexture(veDevice);

    // Load models.
    m_models["cube"] = VeModel::createModelFromFile(veDevice, "assets/models/cube/cube.obj");
    m_models["sphere"] = VeModel::createModelFromFile(veDevice, "assets/models/sphere.obj");

    // Load materials.
    m_materials["default"] = Material::createDefaultMaterial(veDevice);
}


void FirstApp::loadTestScene() {
    // std::shared_ptr<VeTexture> woodTexture =
        // VeTexture::createTextureFromFile(veDevice, "assets/textures/wood.png");
    // std::shared_ptr<VeModel> sphereModel =
    //     VeModel::createModelFromFile(veDevice, "assets/models/sphere.obj");
    // std::shared_ptr<VeModel> cubeModel =
    //     VeModel::createModelFromFile(veDevice, "assets/models/cube/cube.obj");
    // std::shared_ptr<VeModel> monkeyModel =
    //     VeModel::createModelFromFile(veDevice, "assets/models/suzanne.obj");
    // std::shared_ptr<VeModel> bunnyModel =
    //     VeModel::createModelFromFile(veDevice, "assets/models/bunny.obj");
    // std::shared_ptr<VeModel> minecraft =
    //     VeModel::createModelFromFile(veDevice, "assets/models/lost_empire/lost_empire.obj");

    auto cubeObj = VeGameObject::createGameObject();
    cubeObj.model = m_models["cube"];
    cubeObj.transform.scale *= 5.0f;
    cubeObj.material = m_materials["default"];
    // cubeObj.albedoMap = m_textures["empty"];
    // cubeObj.metallicMap = m_textures["empty"];
    // cubeObj.roughnessMap = m_textures["empty"];
    // cubeObj.aoMap = m_textures["empty"];
    gameObjects.emplace(cubeObj.getId(), std::move(cubeObj));

    int numSpheres = 4;

    for (int i = 0; i < numSpheres; i++) {
        for (int j = 0; j < numSpheres; j++) {
            auto x = static_cast<float>(j);
            auto y = static_cast<float>(i);
            auto delta = 1.0f / static_cast<float>(numSpheres);

            float metallic = y * delta;
            float roughness = x * delta + 0.05f;
            if (roughness > 1.0) {
                roughness = 1.0;
            }

            auto sphereObj = VeGameObject::createGameObject();
            sphereObj.model = m_models["sphere"];
            sphereObj.transform.translation = {x * 2.5f, -y * 2.5f, 15.f};
            sphereObj.material = m_materials["default"];
            gameObjects.emplace(sphereObj.getId(), std::move(sphereObj));
        }
    }

    //    auto cubeObj = VeGameObject::createGameObject();
    //    cubeObj.model = cubeModel;
    //    cubeObj.transform.scale *= 20.f;
    //    cubeObj.transform.translation = {0.f, 10.f, 0.f};
    //    cubeObj.texture = woodTexture;
    //    cubeObj.material.roughness = 0.9f;
    //    cubeObj.material.albedo = {0.9f, 0.9f, 0.9f};
    //    cubeObj.material.metallic = 0.0f;
    //    cubeObj.material.ao = 1.f;
    //    gameObjects.emplace(cubeObj.getId(), std::move(cubeObj));
    //
    //    auto monkeyObj = VeGameObject::createGameObject();
    //    monkeyObj.model = monkeyModel;
    //    monkeyObj.texture = woodTexture;
    //    monkeyObj.transform.translation = {-3.f, -3.f, 0.0f};
    //    //    monkeyObj.transform.rotation = {M_PI, M_PI / 2.f, 0.f};
    //    monkeyObj.transform.scale *= 3.0;
    //    monkeyObj.material.roughness = 0.4f;
    //    monkeyObj.material.albedo = {0.4f, 0.6f, 0.9f};
    //    monkeyObj.material.metallic = 0.1f;
    //    monkeyObj.material.ao = 1.f;
    //    gameObjects.emplace(monkeyObj.getId(), std::move(monkeyObj));
    //
    //    auto bunnyObj = VeGameObject::createGameObject();
    //    bunnyObj.model = bunnyModel;
    //    bunnyObj.texture = woodTexture;
    //    bunnyObj.transform.translation = {0.f, 0.f, 4.f};
    //    bunnyObj.transform.rotation = {M_PI, 0.f, 0.f};
    //    bunnyObj.material.roughness = 0.4f;
    //    bunnyObj.material.albedo = {0.0f, 0.6f, 0.0f};
    //    bunnyObj.material.metallic = 0.1f;
    //    bunnyObj.material.ao = 1.f;
    //    gameObjects.emplace(bunnyObj.getId(), std::move(bunnyObj));
    //
    //    auto minecraftObj = VeGameObject::createGameObject();
    //    minecraftObj.model = minecraft;
    //    minecraftObj.transform.rotation.x = M_PI;
    //    minecraftObj.transform.translation.y = 50.f;
    //    minecraftObj.texture = woodTexture;
    //    minecraftObj.material.roughness = 0.9f;
    //    minecraftObj.material.albedo = {0.9f, 0.9f, 0.9f};
    //    minecraftObj.material.metallic = 0.0f;
    //    minecraftObj.material.ao = 1.f;
    //    gameObjects.emplace(minecraftObj.getId(), std::move(minecraftObj));
}

// TODO: Sponza model does not work at all lol
// void FirstApp::initSponzaScene() {
//     std::shared_ptr<VeModel> sponzaModel =
//         VeModel::createModelFromFile(veDevice, "models/sponza/sponza.obj");
//     auto sponzaObj = VeGameObject::createGameObject();
//     sponzaObj.model = sponzaModel;
//     sponzaObj.material.roughness = 0.9f;
//     sponzaObj.material.albedo = {0.9f, 0.9f, 0.9f};
//     sponzaObj.material.metallic = 0.0f;
//     sponzaObj.material.ao = 1.f;
//     gameObjects.emplace(sponzaObj.getId(), std::move(sponzaObj));
// }

}  // namespace ve