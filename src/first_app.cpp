#include "first_app.hpp"

#include "movement_controller.hpp"
#include "systems/point_light_system.hpp"
#include "systems/simple_render_system.hpp"
#include "ve_camera.hpp"
#include "ve_frame_info.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// std
#include <math.h>

#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <stdexcept>

// TODO: Fix gimbal-lock in arcball cam.
// TODO: Prevent arcball zoom into pivot position (will cause runtime-error).

namespace ve {

struct GlobalUbo {
    glm::mat4 projection{1.f};
    glm::mat4 view{1.f};
    glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .01f};  // w is light intensity
    glm::vec3 lightPosition{-2.f, -3.f, 0.5};
    alignas(16) glm::vec4 lightColor{1.f};  // w is light intensity
};

FirstApp::FirstApp() {
    globalPool =
        VeDescriptorPool::Builder(veDevice)
            .setMaxSets(VeSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VeSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();

    loadGameObjects();
    createTextureImage();
}

FirstApp::~FirstApp() {
    // Destroy texture image and it's associated memory.
    vkDestroyImage(veDevice.device(), textureImage, nullptr);
    vkFreeMemory(veDevice.device(), textureImageMemory, nullptr);
}

void FirstApp::run() {
    std::vector<std::unique_ptr<VeBuffer>> uboBuffers(VeSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++) {
        uboBuffers[i] = std::make_unique<VeBuffer>(veDevice,
                                                   sizeof(GlobalUbo),
                                                   1,
                                                   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        uboBuffers[i]->map();
    }

    // Highest level set common to all of our shaders.
    auto globalSetLayout =
        VeDescriptorSetLayout::Builder(veDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
            .build();

    std::vector<VkDescriptorSet> globalDescriptorSets(VeSwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++) {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        VeDescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSets[i]);
    }

    // Initialize the render systems.
    SimpleRenderSystem simpleRenderSystem{
        veDevice, veRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
    PointLightSystem pointLightSystem{
        veDevice, veRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};

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
    float totalTime = 0;
    uint64_t frame = 0;

    bool mousePressed = false;

    // Start game loop.
    while (!veWindow.shouldClose()) {
        // Update delta time.
        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime =
            std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime)
                .count();
        currentTime = newTime;
        totalTime += frameTime;

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
            rotationMatrixY = glm::rotate(
                rotationMatrixY, yAngle, glm::vec3(glm::transpose(camera.getView())[0]));
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
            float r = 2.0f;
            float moveSpeed = 0.5f;
            ubo.lightPosition = {r * std::cos(totalTime * moveSpeed),
                                 -2.0 + std::sin(totalTime * moveSpeed),
                                 r * std::sin(totalTime * moveSpeed)};
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            // render
            veRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(frameInfo);
            pointLightSystem.render(frameInfo);
            veRenderer.endSwapChainRenderPass(commandBuffer);
            veRenderer.endFrame();
        }
    }

    // Wait for GPU to finish before exiting.
    vkDeviceWaitIdle(veDevice.device());

    frame += 1;
}

void FirstApp::loadGameObjects() {
    std::shared_ptr<VeModel> smoothVaseModel =
        VeModel::createModelFromFile(veDevice, "models/smooth_vase.obj");
    auto vaseObj = VeGameObject::createGameObject();
    vaseObj.model = smoothVaseModel;
    vaseObj.transform.translation = {0.f, -1.0f, 0.f};
    vaseObj.transform.scale = {2.5f, 2.0f, 2.0f};
    gameObjects.emplace(vaseObj.getId(), std::move(vaseObj));

    std::shared_ptr<VeModel> cubeModel = VeModel::createModelFromFile(veDevice, "models/cube.obj");
    auto cubeObj = VeGameObject::createGameObject();
    cubeObj.model = cubeModel;
    cubeObj.transform.translation = {0.f, -0.5f, 0.f};
    cubeObj.transform.scale = {0.5f, 0.5f, 0.5f};
    gameObjects.emplace(cubeObj.getId(), std::move(cubeObj));

    std::shared_ptr<VeModel> quadModel = VeModel::createModelFromFile(veDevice, "models/quad.obj");
    auto floorObj = VeGameObject::createGameObject();
    floorObj.model = quadModel;
    floorObj.transform.translation = {0.f, 0.01f, 0.f};
    floorObj.transform.scale = {5.f, 1.f, 5.f};
    gameObjects.emplace(floorObj.getId(), std::move(floorObj));
}

void FirstApp::createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels =
        stbi_load("../textures/statue.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VeBuffer imageStagingBuffer(
        veDevice,
        sizeof(pixels[0]),
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    imageStagingBuffer.map();
    imageStagingBuffer.writeToBuffer(pixels);
    imageStagingBuffer.unmap();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(texWidth);
    imageInfo.extent.height = static_cast<uint32_t>(texHeight);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // Going to be used as destination of our staging buffer and will be sampled in our shaders.
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // idk what this does.
    // Used for multisampling.
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    // Create the image.
    veDevice.createImageWithInfo(
        imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

    // The image data is currently stored in the staging buffer, so we must copy it to our image.
    // Transition the image layout to be optimal for transfering data.
    veDevice.transitionImageLayout(textureImage,
                                   VK_FORMAT_R8G8B8A8_SRGB,
                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy from staging buffer to the image
    veDevice.copyBufferToImage(imageStagingBuffer.getBuffer(),
                               textureImage,
                               static_cast<uint32_t>(texWidth),
                               static_cast<uint32_t>(texHeight),
                               1);

    // Transition the image layout to be optimal for sampling.
    veDevice.transitionImageLayout(textureImage,
                                   VK_FORMAT_R8G8B8A8_SRGB,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // TODO: See if we can move this higher.
    stbi_image_free(pixels);
}

}  // namespace ve