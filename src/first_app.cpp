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

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>

// TODO: Code stuttering after window is changed.
// TODO: Imgui integration
// TODO: Keyboard camera controller.
// TODO: Shadowmapping
// TODO: PBR

namespace ve {

struct GlobalUbo {
    glm::mat4 projection{1.f};
    glm::mat4 view{1.f};
    glm::vec3 lightPosition{4.f, -4.f, 0.0};
    alignas(16) glm::vec3 lightColor{150.f, 150.f, 150.f};
    alignas(16) glm::vec3 viewPos;
};

FirstApp::FirstApp() {
    globalPool =
        VeDescriptorPool::Builder(veDevice)
            .setMaxSets(VeSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VeSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();

    initImgui();
    //    loadGameObjects();
    loadTestScene();
}

FirstApp::~FirstApp() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
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
    VeCamera camera(glm::vec3(4.f, -4.f, 0.f));
    ArcballCam arcCam(veInput, glm::vec3(0.f, 0.f, 0.f));
    MouseCameraController mouseCam(veInput);

    ImGuiIO& io = ImGui::GetIO();

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
        if (veInput.getMouseButton(GLFW_MOUSE_BUTTON_LEFT) && !io.WantCaptureMouse) {
            veInput.setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            mouseCam.update(camera, frameTime);
        } else {
            veInput.setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        auto aspect = veRenderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, .1, 100);

        // Imgui new frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

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
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

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

    std::shared_ptr<VeModel> sphereModel =
        VeModel::createModelFromFile(veDevice, "models/sphere.obj");
    auto sphereObj = VeGameObject::createGameObject();
    sphereObj.model = sphereModel;
    sphereObj.transform.translation = {-1.5f, -1.5f, -1.5f};
    sphereObj.texture = woodTexture;
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
        VeTexture::createTextureFromFile(veDevice, "textures/wood.png");

    std::shared_ptr<VeModel> sphereModel =
        VeModel::createModelFromFile(veDevice, "models/sphere.obj");
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
            sphereObj.texture = woodTexture;
            sphereObj.transform.translation = {x * 2.5f, -y * 2.5f, 15.f};
            sphereObj.material.metallic = metallic;
            sphereObj.material.roughness = roughness;
            gameObjects.emplace(sphereObj.getId(), std::move(sphereObj));
        }
    }
}

// Resources used:
// - https://vkguide.dev/docs/extra-chapter/implementing_imgui/
// - https://frguthmann.github.io/posts/vulkan_imgui/
void FirstApp::initImgui() {
    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Imgui style.
    ImGui::StyleColorsDark();

    // Create descriptor pool for Imgui.
    std::vector<VkDescriptorPoolSize> poolSizes({{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                                 {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                                 {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                                 {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                                 {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                                 {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                                 {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                                 {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                                 {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                                 {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                                 {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}});
    imguiPool = std::make_unique<VeDescriptorPool>(
        veDevice, 1000, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, poolSizes);

    // Setup platform/renderer bindings.
    ImGui_ImplGlfw_InitForVulkan(veWindow.getGLFWWindow(), true);
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = veDevice.getInstance();
    initInfo.PhysicalDevice = veDevice.getPhysicalDevice();
    initInfo.Device = veDevice.device();
    initInfo.Queue = veDevice.graphicsQueue();
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = imguiPool->pool();
    initInfo.Allocator = nullptr;
    initInfo.MinImageCount = veRenderer.getSwapChainImageCount();
    initInfo.ImageCount = veRenderer.getSwapChainImageCount();
    // TODO: Change this later when doing multisampling?
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&initInfo, veRenderer.getSwapChainRenderPass());

    // Setup font stuff.
    VkCommandBuffer cmdBuffer = veDevice.beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer);
    veDevice.endSingleTimeCommands(cmdBuffer);

    // Clear font textures from cpu data.
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

}  // namespace ve