#include "ve_imgui.h"

namespace ve {

// Resources used:
// - https://vkguide.dev/docs/extra-chapter/implementing_imgui/
// - https://frguthmann.github.io/posts/vulkan_imgui/
VeImGui::VeImGui(VeRenderer &veRenderer) {
    VeDevice &veDevice = veRenderer.getDevice();
    VeWindow &veWindow = veRenderer.getWindow();

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

VeImGui::~VeImGui() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void VeImGui::beginFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}
void VeImGui::render(VkCommandBuffer cmdBuffer) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
}



}  // namespace ve