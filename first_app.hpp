#pragma once

#include <memory>
#include <vector>

#include "ve_device.hpp"
#include "ve_model.hpp"
#include "ve_pipeline.hpp"
#include "ve_swap_chain.hpp"
#include "ve_window.hpp"

namespace ve {

class FirstApp {
   public:
    static constexpr int WIDTH = 800;
    static constexpr int HEIGHT = 600;

    FirstApp();
    ~FirstApp();

    // Remove copy constructors.
    FirstApp(const FirstApp &) = delete;
    FirstApp &operator=(const FirstApp &) = delete;

    void run();

   private:
    void loadModels();
    void createPipelineLayout();
    void createPipeline();
    void createCommandBuffers();
    void drawFrame();
    void recreateSwapChain();
    void recordCommandBuffer(int imageIndex);

    VeWindow veWindow{WIDTH, HEIGHT, "Hello Vulkan!"};
    VeDevice veDevice{veWindow};
    std::unique_ptr<VeSwapChain> veSwapChain;
    std::unique_ptr<VePipeline> vePipeline;
    VkPipelineLayout pipelineLayout;
    std::vector<VkCommandBuffer> commandBuffers;
    std::unique_ptr<VeModel> veModel;
};

}  // namespace ve