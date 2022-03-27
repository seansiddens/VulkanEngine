#pragma once

// std
#include <memory>
#include <vector>

#include "Core/ve_game_object.hpp"
#include "Core/ve_input.hpp"
#include "Core/ve_window.hpp"
#include "ImGui/ve_imgui.h"
#include "Renderer/ve_descriptors.hpp"
#include "Renderer/ve_device.hpp"
#include "Renderer/ve_renderer.hpp"

namespace ve {

class FirstApp {
   public:
    static constexpr int WIDTH = 1280;
    static constexpr int HEIGHT = 720;

    FirstApp();
    ~FirstApp() = default;

    // Remove copy constructors.
    FirstApp(const FirstApp &) = delete;
    FirstApp &operator=(const FirstApp &) = delete;

    void run();

   private:
    void initScene();
    void loadAssets();
    void loadGameObjects();
    void loadTestScene();
    void initSponzaScene();

   private:
    // NOTE: These classes need to be initialized in this order.
    VeWindow veWindow{WIDTH, HEIGHT, "Vulkan Engine"};
    VeInput veInput{veWindow};
    VeDevice veDevice{veWindow};
    VeRenderer veRenderer{veWindow, veDevice};
    VeImGui veImGui{veRenderer};

    std::unique_ptr<VeDescriptorPool> globalPool{};
    VeGameObject::Map gameObjects;
    std::shared_ptr<VeTexture> m_cubemap;
    
    // Assets
    std::unordered_map<std::string, std::shared_ptr<VeTexture>> m_textures;
    std::unordered_map<std::string, std::shared_ptr<VeModel>> m_models;
    std::unordered_map<std::string, std::shared_ptr<Material>> m_materials;

};

}  // namespace ve