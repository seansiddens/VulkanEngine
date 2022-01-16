#pragma once

#include <string>
#include <vector>

#include "ve_device.hpp"

namespace ve {

// Decoupled from our VePipeline class so that the application layer can configure every
// aspect of a pipeline and a particular configuration can be shared across multiple
// pipeline instances.
struct PipelineConfigInfo {
    // Remove copy constructors.
    PipelineConfigInfo(const PipelineConfigInfo&) = delete;
    PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

    // Contains information about the viewport and the scissor.
    // The viewport describes how to transform from our gl_Position (-1 to 1) to pixels
    // in the image we are rendering to (0 to WIDTH/HEIHT).
    // Every fragment is compared against a scissor rectangle before being drawn.
    // If it lies outside the scissor, it is not rendered.
    VkPipelineViewportStateCreateInfo viewportInfo;

    // The input assembler is the first stage of our graphics pipeline. It groups our
    // list of vertices into primitives. The type of primitive (triangle, triangle strip,
    // line, etc) is configured w/ the 'topology' field of the struct.
    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/chap21.html#VkPipelineInputAssemblyStateCreateInfo
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;

    // The rasterization stage breaks up our geometry into pixels for each fragment our primitives
    // overlap.
    VkPipelineRasterizationStateCreateInfo rasterizationInfo;

    // TODO(sean): Document more info on what this is.
    VkPipelineMultisampleStateCreateInfo multisampleInfo;

    // TODO(sean): Document more info on what this is.
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlendInfo;

    // For each pixel, the depth buffer stores the depth of the closet fragment. When
    // drawing, fragments behind this are discarded.
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo;

    std::vector<VkDynamicState> dynamicStateEnables;
    VkPipelineDynamicStateCreateInfo dynamicStateInfo;

    // These values are initialized by the application layer.
    VkPipelineLayout pipelineLayout = nullptr;
    VkRenderPass renderPass = nullptr;
    uint32_t subpass = 0;
};

class VePipeline {
   public:
    VePipeline(VeDevice& device,
               const std::string& vertFilepath,
               const std::string& fragFilepath,
               const PipelineConfigInfo& configInfo);

    ~VePipeline();

    // Delete copy constructors to avoid accidentally duplicating pointers to our Vulkan
    // objects.
    VePipeline(const VePipeline&) = delete;
    VePipeline& operator=(const VePipeline&) = delete;

    void bind(VkCommandBuffer commandBuffer);

    // Initializes a default pipeline configuration.
    static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);

   private:
    // Returns a buffer containing the contents of a file.
    static std::vector<char> readFile(const std::string& filepath);

    void createGraphicsPipeline(const std::string& vertFilepath,
                                const std::string& fragFilepath,
                                const PipelineConfigInfo& configInfo);

    void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

    // Potentially memory unsafe if our device is freed before our pipeline.
    // Reference member will implicitly outlive the class since a pipeline MUST have a
    // device to exist (aggregation relationship).
    VeDevice& veDevice;
    VkPipeline graphicsPipeline;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
};

}  // namespace ve