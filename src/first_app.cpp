#include "first_app.hpp"

#include "Core/camera_controller.hpp"
#include "Core/movement_controller.hpp"
#include "Core/ve_camera.hpp"
#include "Core/ve_frame_info.hpp"
#include "Renderer/ve_texture.hpp"
#include "systems/point_light_system.hpp"
#include "systems/simple_render_system.hpp"

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

FirstApp::FirstApp() {
    globalPool =
        VeDescriptorPool::Builder(veDevice)
            .setMaxSets(VeSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VeSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();

    //    loadGameObjects();
    //    loadTestScene();
    initScene();
}

void FirstApp::run() {
    // Set clear color.
    veRenderer.setClearColor({0.05, 0.05, 0.05, 1.f});

    // Create uniform buffer objects.
    std::vector<std::unique_ptr<VeBuffer>> uboBuffers(VeSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (auto& uboBuffer : uboBuffers) {
        uboBuffer = std::make_unique<VeBuffer>(veDevice,
                                               sizeof(GlobalUbo),
                                               1,
                                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        uboBuffer->map();
    }

    // Highest level set common to all of our shaders.
    auto globalSetLayout =
        VeDescriptorSetLayout::Builder(veDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
            .build();

    std::vector<VkDescriptorSet> globalDescriptorSets(VeSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++) {
        // UBO info.
        auto bufferInfo = uboBuffers[i]->descriptorInfo();

        // Write to descriptor.
        VeDescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSets[i]);
    }

    // Initialize the render systems.
    SimpleRenderSystem simpleRenderSystem{veDevice,
                                          veRenderer.getSwapChainRenderPass(),
                                          globalSetLayout->getDescriptorSetLayout(),
                                          gameObjects};
    PointLightSystem pointLightSystem{
        veDevice, veRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};

    // Initialize the camera and camera controller.
    VeCamera camera(glm::vec3(4.f, -4.f, 0.f), glm::vec3(0.f, 0.f, 15.f));
    ArcballCam arcCam(veInput, glm::vec3(0.f, 0.f, 0.f));
    MouseCameraController mouseCam(veInput, 25.f, 0.5f);
    KeyboardCameraController keyCam(veInput, 25.f, 2.f);

    // Initialize the current time.
    auto currentTime = std::chrono::high_resolution_clock::now();
    float totalTime = 0;  // Total elapsed time of the application.
    uint64_t frame = 0;   // Current frame.

    // Start game loop.
    while (!veWindow.shouldClose()) {
        // Update delta time.
        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime =
            std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime)
                .count();
        currentTime = newTime;
        totalTime += frameTime;

        // Poll events.
        veInput.pollEvents();
        if (veInput.getKey(GLFW_KEY_ESCAPE)) break;

        // Only update camera when mouse button is held.
        if (veInput.getMouseButton(GLFW_MOUSE_BUTTON_LEFT) && !VeImGui::wantMouse()) {
            veInput.setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            mouseCam.update(camera, frameTime);
        } else {
            veInput.setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            keyCam.update(camera, frameTime);
        }

        auto aspect = veRenderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, .1, 1000);

        // Imgui new frame
        VeImGui::beginFrame();

        // Imgui commands.
        ImGui::ShowDemoWindow();

        // Finalize the ImGui frame and prepare draw data.
        ImGui::Render();

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
    loadTestScene();
    //    initSponzaScene();
}

void FirstApp::loadGameObjects() {
    // Load textures.
    std::shared_ptr<VeTexture> statueTexture =
        VeTexture::createTextureFromFile(veDevice, "assets/textures/statue.jpg");

    std::shared_ptr<VeTexture> vikingTexture =
        VeTexture::createTextureFromFile(veDevice, "assets/textures/viking_room.png");

    std::shared_ptr<VeTexture> woodTexture =
        VeTexture::createTextureFromFile(veDevice, "textures/wood.png");

    // Load models and attach them to game objects.
    std::shared_ptr<VeModel> smoothVaseModel =
        VeModel::createModelFromFile(veDevice, "models/smooth_vase.obj");
    auto vaseObj = VeGameObject::createGameObject();
    vaseObj.model = smoothVaseModel;
    vaseObj.transform.translation = {0.f, -1.0f, 0.f};
    vaseObj.transform.scale = {2.0f, 2.0f, 2.0f};
    vaseObj.texture = woodTexture;
    gameObjects.emplace(vaseObj.getId(), std::move(vaseObj));

    std::shared_ptr<VeModel> cubeModel = VeModel::createModelFromFile(veDevice, "models/cube.obj");
    auto cubeObj = VeGameObject::createGameObject();
    cubeObj.model = cubeModel;
    cubeObj.texture = statueTexture;
    cubeObj.transform.translation = {0.f, -0.5f, 0.f};
    cubeObj.transform.scale = {0.5f, 0.5f, 0.5f};
    gameObjects.emplace(cubeObj.getId(), std::move(cubeObj));
    //
    std::shared_ptr<VeModel> quadModel = VeModel::createModelFromFile(veDevice, "models/quad.obj");
    auto floorObj = VeGameObject::createGameObject();
    floorObj.model = quadModel;
    floorObj.transform.translation = {0.f, 0.01f, 0.f};
    floorObj.transform.scale = {10.f, 1.f, 10.f};
    floorObj.texture = woodTexture;
    gameObjects.emplace(floorObj.getId(), std::move(floorObj));

    std::shared_ptr<VeModel> sphereModel =
        VeModel::createModelFromFile(veDevice, "models/sphere.obj");
    auto sphereObj = VeGameObject::createGameObject();
    sphereObj.model = sphereModel;
    sphereObj.transform.translation = {-1.5f, -1.5f, -1.5f};
    gameObjects.emplace(sphereObj.getId(), std::move(sphereObj));

    // std::shared_ptr<VeModel> vikingModel =
    //     VeModel::createModelFromFile(veDevice, "models/viking_room.obj");
    // auto vikingObj = VeGameObject::createGameObject();
    // vikingObj.model = vikingModel;
    // vikingObj.transform.rotation.x = M_PI / 2.f;
    // vikingObj.transform.translation.y -= 0.5f;
    // vikingObj.texture = vikingTexture;
    // gameObjects.emplace(vikingObj.getId(), std::move(vikingObj));
}

void FirstApp::loadTestScene() {
    std::shared_ptr<VeTexture> woodTexture =
        VeTexture::createTextureFromFile(veDevice, "assets/textures/wood.png");
    std::shared_ptr<VeTexture> albedoTexture = VeTexture::createTextureFromFile(
        veDevice, "assets/materials/rusted_iron2/rustediron2_albedo.png");
    std::shared_ptr<VeTexture> metallicTexture = VeTexture::createTextureFromFile(
        veDevice, "assets/materials/rusted_iron2/rustediron2_metallic.png");
    std::shared_ptr<VeTexture> roughnessTexture = VeTexture::createTextureFromFile(
        veDevice, "assets/materials/rusted_iron2/rustediron2_roughness.png");
    std::shared_ptr<VeTexture> emptyTexture = VeTexture::createEmptyTexture(veDevice);

    std::shared_ptr<VeModel> sphereModel =
        VeModel::createModelFromFile(veDevice, "assets/models/sphere.obj");
    std::shared_ptr<VeModel> cubeModel =
        VeModel::createModelFromFile(veDevice, "assets/models/cube/cube.obj");
    std::shared_ptr<VeModel> monkeyModel =
        VeModel::createModelFromFile(veDevice, "assets/models/suzanne.obj");
    std::shared_ptr<VeModel> bunnyModel =
        VeModel::createModelFromFile(veDevice, "assets/models/bunny.obj");
    std::shared_ptr<VeModel> minecraft =
        VeModel::createModelFromFile(veDevice, "assets/models/lost_empire/lost_empire.obj");

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
            sphereObj.model = sphereModel;
            sphereObj.texture = emptyTexture;
            sphereObj.albedoMap = albedoTexture;
//            sphereObj.metallicMap = emptyTexture;
            sphereObj.metallicMap = metallicTexture;
            sphereObj.material.metallic = 1.0;
            sphereObj.roughnessMap = roughnessTexture;
            sphereObj.aoMap = emptyTexture;
            sphereObj.transform.translation = {x * 2.5f, -y * 2.5f, 15.f};
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
void FirstApp::initSponzaScene() {
    std::shared_ptr<VeModel> sponzaModel =
        VeModel::createModelFromFile(veDevice, "models/sponza/sponza.obj");
    auto sponzaObj = VeGameObject::createGameObject();
    sponzaObj.model = sponzaModel;
    sponzaObj.material.roughness = 0.9f;
    sponzaObj.material.albedo = {0.9f, 0.9f, 0.9f};
    sponzaObj.material.metallic = 0.0f;
    sponzaObj.material.ao = 1.f;
    gameObjects.emplace(sponzaObj.getId(), std::move(sponzaObj));
}

}  // namespace ve