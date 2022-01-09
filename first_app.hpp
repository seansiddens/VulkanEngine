#pragma once

// std
#include <memory>
#include <vector>

#include "ve_device.hpp"
#include "ve_game_object.hpp"
#include "ve_renderer.hpp"
#include "ve_window.hpp"

namespace ve {

class FirstApp {
   public:
    static constexpr int WIDTH = 1280;
    static constexpr int HEIGHT = 720;

    FirstApp();
    ~FirstApp();

    // Remove copy constructors.
    FirstApp(const FirstApp &) = delete;
    FirstApp &operator=(const FirstApp &) = delete;

    void run();

   private:
    void loadGameObjects();

    VeWindow veWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
    VeDevice veDevice{veWindow};
    VeRenderer veRenderer{veWindow, veDevice};

    std::vector<VeGameObject> gameObjects;
};

}  // namespace ve