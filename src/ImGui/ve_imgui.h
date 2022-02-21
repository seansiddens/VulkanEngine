#pragma once

#include "Renderer/ve_descriptors.hpp"
#include "Renderer/ve_renderer.hpp"

// lib
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace ve {

class VeImGui {
   public:
    explicit VeImGui(VeRenderer& veRenderer);
    ~VeImGui();

    // Returns whether ImGui is handling a keyboard event.
    [[nodiscard]] static bool wantKeyboard() { return ImGui::GetIO().WantCaptureKeyboard;}
    // Returns whether ImGui is handling a mouse event.
    [[nodiscard]] static bool wantMouse() { return ImGui::GetIO().WantCaptureMouse; }

    static void beginFrame();
    static void render(VkCommandBuffer cmdBuffer);

   private:
    std::unique_ptr<VeDescriptorPool> imguiPool{};
};

}  // namespace ve
