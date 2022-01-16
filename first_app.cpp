#include "first_app.hpp"

#include "movement_controller.hpp"
#include "simple_render_system.hpp"
#include "ve_camera.hpp"
#include "ve_frame_info.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <math.h>

#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace ve {

struct GlobalUbo {
    glm::mat4 projectionView{1.f};
    glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .02f}; // w is light intensity
    glm::vec3 lightPosition{-1.f};
    alignas(16) glm::vec4 lightColor{1.f}; // w is light intensity
};

FirstApp::FirstApp() {
    globalPool =
        VeDescriptorPool::Builder(veDevice)
            .setMaxSets(VeSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VeSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();

    loadGameObjects();
}

FirstApp::~FirstApp() {}

void FirstApp::run() {
    std::vector<std::unique_ptr<VeBuffer>> uboBuffers(VeSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++) {
        uboBuffers[i] = std::make_unique<VeBuffer>(veDevice, sizeof(GlobalUbo), 1,
                                                   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        uboBuffers[i]->map();
    }

    // Highest level set common to all of our shaders.
    auto globalSetLayout =
        VeDescriptorSetLayout::Builder(veDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .build();

    std::vector<VkDescriptorSet> globalDescriptorSets(VeSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++) {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        VeDescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSets[i]);
    }

    // Initialize the render system.
    SimpleRenderSystem simpleRenderSystem{veDevice, veRenderer.getSwapChainRenderPass(),
                                          globalSetLayout->getDescriptorSetLayout()};

    // Initialize the camera and camera controller.
    VeCamera camera{};
    auto viewerObject = VeGameObject::createGameObject();
    viewerObject.transform.translation = glm::vec3{0.f, 99.f, -3.f};
    // viewerObject.transform.rotation = glm::vec3{glm::pi<float>() * 0.125, 0.f, 0.f};
    KeyboardMovementController cameraController{};

    glm::vec4 pivot{0.f, -1.0f, 0.f, 1.f};
    glm::vec4 cameraPos{0.f, -1.f, -3.f, 1.f};

    double lastMouseX;
    double lastMouseY;
    glfwGetCursorPos(veWindow.getGLFWWindow(), &lastMouseX, &lastMouseY);

    // Initialize the current time.
    auto currentTime = std::chrono::high_resolution_clock::now();
    bool mousePressed = false;

    // Start game loop.
    while (!veWindow.shouldClose()) {
        // Update delta time.
        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime =
            std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime)
                .count();
        currentTime = newTime;

        glm::vec3 forwardDir = glm::normalize(pivot - cameraPos);

        // Poll events.
        glfwPollEvents();
        if (glfwGetKey(veWindow.getGLFWWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) break;
        if (glfwGetMouseButton(veWindow.getGLFWWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            mousePressed = true;
        } else {
            mousePressed = false;
        }

        const float zoomSpeed = 2.5f;
        if (glfwGetKey(veWindow.getGLFWWindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            cameraPos += glm::vec4(zoomSpeed * frameTime * forwardDir, 0.f);
        }
        if (glfwGetKey(veWindow.getGLFWWindow(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
            cameraPos -= glm::vec4(zoomSpeed * frameTime * forwardDir, 0.f);
        }

        // A movement from left to right = 2 * PI = 360 deg
        float deltaAngleX = (2.f * M_PI / static_cast<float>(veWindow.getExtent().width));
        // A movement from top to bottom = PI = 180 deg.
        float deltaAngleY = (M_PI / static_cast<float>(veWindow.getExtent().height));

        double currMouseX, currMouseY;
        glfwGetCursorPos(veWindow.getGLFWWindow(), &currMouseX, &currMouseY);

        float xAngle = (lastMouseX - currMouseX) * deltaAngleX;
        float yAngle = (lastMouseY - currMouseY) * deltaAngleY;

        // Extra step to handle the problem when the camera direction is the same as the up vector
        // float cosAngle = glm::dot(-glm::vec3(glm::transpose(viewerObject.transform.mat4())[0]),
        //                           glm::vec3{0.f, -1.f, 0.f});

        if (mousePressed) {
            // Rotate camera object around pivot point about the Y axis.
            glm::mat4 rotMatrixX(1.f);
            rotMatrixX = glm::rotate(rotMatrixX, xAngle, glm::vec3{0.f, -1.f, 0.f});
            cameraPos = (rotMatrixX * (cameraPos - pivot)) + pivot;

            // Rotate camera around pivot about the camera object's right dir.
            glm::mat4 rotationMatrixY(1.0f);
            rotationMatrixY = glm::rotate(rotationMatrixY, yAngle,
                                          glm::vec3(glm::transpose(camera.getView())[0]));
            glm::vec3 finalPosition = (rotationMatrixY * (cameraPos - pivot)) + pivot;
            cameraPos = glm::vec4(finalPosition, 1.f);
        }

        lastMouseX = currMouseX;
        lastMouseY = currMouseY;

        // Move camera
        // cameraController.moveInPlaneXZ(veWindow.getGLFWWindow(), frameTime, viewerObject);
        // camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

        camera.setViewTarget(cameraPos, pivot);

        auto aspect = veRenderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, .1, 100);

        // beginFrame() will return a nullptr if swap chain needs to be recreated (window resized).
        if (auto commandBuffer = veRenderer.beginFrame()) {
            int frameIndex = veRenderer.getFrameIndex();
            FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera,
                                globalDescriptorSets[frameIndex]};

            // update
            GlobalUbo ubo{};
            ubo.projectionView = camera.getProjection() * camera.getView();
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            // render
            veRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(frameInfo, gameObjects);
            veRenderer.endSwapChainRenderPass(commandBuffer);
            veRenderer.endFrame();
        }
    }

    // Wait for GPU to finish before exiting.
    vkDeviceWaitIdle(veDevice.device());
}

void FirstApp::loadGameObjects() {

    std::shared_ptr<VeModel> smoothVaseModel =
        VeModel::createModelFromFile(veDevice, "models/smooth_vase.obj");
    auto vaseObj = VeGameObject::createGameObject();
    vaseObj.model = smoothVaseModel;
    vaseObj.transform.translation = {0.f, -1.0f, 0.f};
    vaseObj.transform.scale = {2.5f, 2.0f, 2.0f};
    gameObjects.push_back(std::move(vaseObj));

    std::shared_ptr<VeModel> cubeModel = VeModel::createModelFromFile(veDevice, "models/cube.obj");
    auto cubeObj = VeGameObject::createGameObject();
    cubeObj.model = cubeModel;
    cubeObj.transform.translation = {0.f, -0.5f, 0.f};
    cubeObj.transform.scale = {0.5f, 0.5f, 0.5f};
    gameObjects.push_back(std::move(cubeObj));

    std::shared_ptr<VeModel> quadModel = VeModel::createModelFromFile(veDevice, "models/quad.obj");
    auto floorObj = VeGameObject::createGameObject();
    floorObj.model = quadModel;
    floorObj.transform.translation = {0.f, 0.f, 0.f};
    floorObj.transform.scale = {5.f, 1.f, 5.f};
    gameObjects.push_back(std::move(floorObj));
}

}  // namespace ve