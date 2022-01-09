#include "first_app.hpp"

#include "movement_controller.hpp"
#include "simple_render_system.hpp"
#include "ve_camera.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace ve {

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    MouseMovementController* mouseController =
        static_cast<MouseMovementController*>(glfwGetWindowUserPointer(window));

    if (mouseController->firstMouse) {
        mouseController->lastX = xpos;
        mouseController->lastY = ypos;
        mouseController->firstMouse = false;
    }

    double xOffset = xpos - mouseController->lastX;
    double yOffset = mouseController->lastY - ypos;
    mouseController->lastX = xpos;
    mouseController->lastY = ypos;

    mouseController->updateObject(xOffset, yOffset);
}

FirstApp::FirstApp() { loadGameObjects(); }

FirstApp::~FirstApp() {}

void FirstApp::run() {
    // Check the maximum size of push constants available on the device.
    std::cout << "maxPushConstantsSize  = " << veDevice.properties.limits.maxPushConstantsSize
              << "\n";

    // Initialize the render system.
    SimpleRenderSystem simpleRenderSystem{veDevice, veRenderer.getSwapChainRenderPass()};

    // Initialize the camera and camera controller.
    VeCamera camera{};
    auto viewerObject = VeGameObject::createGameObject();
    KeyboardMovementController cameraController{};

    // Initialize the current time.
    auto currentTime = std::chrono::high_resolution_clock::now();

    // Start game loop.
    while (!veWindow.shouldClose()) {
        // Poll events.
        glfwPollEvents();
        if (glfwGetKey(veWindow.getGLFWWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

        // Update delta time.
        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime =
            std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime)
                .count();
        currentTime = newTime;

        // Move camera
        cameraController.moveInPlaneXZ(veWindow.getGLFWWindow(), frameTime, viewerObject);
        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

        auto aspect = veRenderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, .1, 100);

        // beginFrame() will return a nullptr if swap chain needs to be recreated (window resized).
        if (auto commandBuffer = veRenderer.beginFrame()) {
            veRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
            veRenderer.endSwapChainRenderPass(commandBuffer);
            veRenderer.endFrame();
        }
    }

    // Wait for GPU to finish before exiting.
    vkDeviceWaitIdle(veDevice.device());
}

void FirstApp::loadGameObjects() {
    std::shared_ptr<VeModel> veModel =
        VeModel::createModelFromFile(veDevice, "models/smooth_vase.obj");

    auto gameObj = VeGameObject::createGameObject();
    gameObj.model = veModel;
    gameObj.transform.translation = {0.f, 0.f, 3.5f};
    gameObj.transform.scale = {2.5f, 2.5f, 2.5f};

    gameObjects.push_back(std::move(gameObj));
}

}  // namespace ve