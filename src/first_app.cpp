#include "first_app.hpp"

#include "camera_controller.hpp"
#include "movement_controller.hpp"
#include "systems/point_light_system.hpp"
#include "systems/simple_render_system.hpp"
#include "ve_camera.hpp"
#include "ve_frame_info.hpp"
#include "ve_texture.hpp"

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

// TODO: Material abstraction?
// TODO: Code stuttering after window is changed.
// TODO: Imgui integration
// TODO: Keyboard camera controller and FPS camera controller.

namespace ve {

struct GlobalUbo {
    glm::mat4 projection{1.f};
    glm::mat4 view{1.f};
    glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .1f};  // w is light intensity
    glm::vec3 lightPosition{0.f, -3.f, 0.0};
    alignas(16) glm::vec4 lightColor{1.f, 1.f, 1.f, 5.f};  // w is light intensity
};

FirstApp::FirstApp() {
    globalPool =
        VeDescriptorPool::Builder(veDevice)
            .setMaxSets(VeSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VeSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();

    loadGameObjects();
}

void FirstApp::run() {
    // Set clear color.
    veRenderer.setClearColor({0.2, 0.2, 0.6, 1.f});

    // Create uniform buffer objects.
    std::vector<std::unique_ptr<VeBuffer>> uboBuffers(VeSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (auto &uboBuffer : uboBuffers) {
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
    VeCamera camera(glm::vec3(0.f, 0.f, -3.f), glm::vec3(0.f, 0.f, 0.f));
    ArcballCam arcCam(veInput, glm::vec3(0.f, 0.f, 0.f));

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

        // Update camera position.
        arcCam.update(camera, frameTime);

        auto aspect = veRenderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, .1, 100);

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
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            // Begin render pass.
            veRenderer.beginSwapChainRenderPass(commandBuffer);

            simpleRenderSystem.renderGameObjects(frameInfo);
            pointLightSystem.render(frameInfo);

            // End render pass and frame.
            veRenderer.endSwapChainRenderPass(commandBuffer);
            veRenderer.endFrame();
        }
    }

    // Wait for GPU to finish before exiting.
    vkDeviceWaitIdle(veDevice.device());

    frame += 1;
}

void FirstApp::loadGameObjects() {
    // Load textures.
    std::shared_ptr<VeTexture> statueTexture =
        VeTexture::createTextureFromFile(veDevice, "textures/statue.jpg");

    std::shared_ptr<VeTexture> vikingTexture =
        VeTexture::createTextureFromFile(veDevice, "textures/viking_room.png");

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

    // std::shared_ptr<VeModel> vikingModel =
    //     VeModel::createModelFromFile(veDevice, "models/viking_room.obj");
    // auto vikingObj = VeGameObject::createGameObject();
    // vikingObj.model = vikingModel;
    // vikingObj.transform.rotation.x = M_PI / 2.f;
    // vikingObj.transform.translation.y -= 0.5f;
    // vikingObj.texture = vikingTexture;
    // gameObjects.emplace(vikingObj.getId(), std::move(vikingObj));
}

}  // namespace ve