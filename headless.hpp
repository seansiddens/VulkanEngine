
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

class Headless {
   public:
    static constexpr int WIDTH = 1280;
    static constexpr int HEIGHT = 720;

    Headless();
    ~Headless() = default;

    // Remove copy constructors.
    Headless(const FirstApp &) = delete;
    Headless &operator=(const FirstApp &) = delete;

    void run();

   private:

   private:
    // NOTE: These classes need to be initialized in this order.
    VeWindow veWindow{WIDTH, HEIGHT, "Vulkan Engine"};
    VeInput veInput{veWindow};
    VeDevice veDevice{veWindow};
    VeRenderer veRenderer{veWindow, veDevice};
    VeImGui veImGui{veRenderer};

    std::unique_ptr<VeDescriptorPool> globalPool{};
};

}  // namespace ve