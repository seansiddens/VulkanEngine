#pragma once

#include "ve_device.hpp"
#include "ve_pipeline.hpp"
#include "ve_window.hpp"

namespace ve {

class FirstApp {
   public:
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 800;

    void run();

   private:
    VeWindow veWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
    VeDevice veDevice{veWindow};
    VePipeline vePipeline{veDevice, "shaders/simple_shader.vert.spv",
                          "shaders/simple_shader.frag.spv",
                          VePipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)};
};

}  // namespace ve