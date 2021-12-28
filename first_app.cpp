#include "first_app.hpp"

#include "simple_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace ve {

FirstApp::FirstApp() { loadGameObjects(); }

FirstApp::~FirstApp() {}

void FirstApp::run() {
    // Check the maximum size of push constants available on the device.
    std::cout << "maxPushConstantsSize  = " << veDevice.properties.limits.maxPushConstantsSize
              << "\n";

    SimpleRenderSystem simpleRenderSystem{veDevice, veRenderer.getSwapChainRenderPass()};

    while (!veWindow.shouldClose()) {
        glfwPollEvents();

        // beginFrame() will return a nullptr if swap chain needs to be recreated (window resized).
        if (auto commandBuffer = veRenderer.beginFrame()) {
            veRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects);
            veRenderer.endSwapChainRenderPass(commandBuffer);
            veRenderer.endFrame();
        }
    }

    // Wait for GPU to finish before exiting.
    vkDeviceWaitIdle(veDevice.device());
}

void FirstApp::loadGameObjects() {
    std::vector<VeModel::Vertex> vertices{{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                          {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                          {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};
    auto veModel = std::make_shared<VeModel>(veDevice, vertices);

    std::vector<glm::vec3> colors{
        {1.f, .7f, .73f},
        {1.f, .87f, .73f},
        {1.f, 1.f, .73f},
        {.73f, 1.f, .8f},
        {.73, .88f, 1.f}  //
    };
    for (auto& color : colors) {
        color = glm::pow(color, glm::vec3{2.2f});
    }
    for (int i = 0; i < 40; i++) {
        auto triangle = VeGameObject::createGameObject();
        triangle.model = veModel;
        triangle.transform2d.scale = glm::vec2(.5f) + i * 0.025f;
        triangle.transform2d.rotation = i * glm::pi<float>() * .025f;
        triangle.color = colors[i % colors.size()];
        gameObjects.push_back(std::move(triangle));
    }
}

}  // namespace ve