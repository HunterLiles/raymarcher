#include "pipelines.hpp"
#include "shaders.hpp"
#include <algorithm>

VkPipeline compute_pipeline_create(const Device &d, VkPipelineLayout layout,
                                   const std::filesystem::path &path) {
    VkShaderModule shader = shader_load(d.handle, path);
    VkPipeline pipeline{};

    try {
        VkComputePipelineCreateInfo ci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        ci.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ci.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        ci.stage.module = shader;
        ci.stage.pName = "main";
        ci.layout = layout;

        VK_CHECK(vkCreateComputePipelines(d.handle, VK_NULL_HANDLE, 1, &ci, nullptr, &pipeline));
    } catch (...) {
        if (pipeline)
            vkDestroyPipeline(d.handle, pipeline, nullptr);

        vkDestroyShaderModule(d.handle, shader, nullptr);
        throw;
    }

    vkDestroyShaderModule(d.handle, shader, nullptr);
    return pipeline;
}

void pipeline_create(const Device &d, Pipeline &p, const std::filesystem::path &path) {
    try {
        VkDescriptorSetLayoutBinding bindings[] = {
            {0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
            {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        };

        VkDescriptorSetLayoutCreateInfo descriptors{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        descriptors.bindingCount = 2;
        descriptors.pBindings = bindings;

        VK_CHECK(
            vkCreateDescriptorSetLayout(d.handle, &descriptors, nullptr, &p.descriptor_layout));

        uint32_t push_bytes = uint32_t(std::max(sizeof(FrameData), sizeof(CacheDispatch)));

        if (push_bytes > d.properties.limits.maxPushConstantsSize)
            throw std::runtime_error("Push constants exceed device limit");

        VkPushConstantRange range{VK_SHADER_STAGE_COMPUTE_BIT, 0, push_bytes};

        VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layout.setLayoutCount = 1;
        layout.pSetLayouts = &p.descriptor_layout;
        layout.pushConstantRangeCount = 1;
        layout.pPushConstantRanges = &range;

        VK_CHECK(vkCreatePipelineLayout(d.handle, &layout, nullptr, &p.layout));
        p.handle = compute_pipeline_create(d, p.layout, path);
    } catch (...) {
        pipeline_destroy(d, p);
        throw;
    }
}

void pipeline_destroy(const Device &d, Pipeline &p) {
    if (p.handle)
        vkDestroyPipeline(d.handle, p.handle, nullptr);

    if (p.layout)
        vkDestroyPipelineLayout(d.handle, p.layout, nullptr);

    if (p.descriptor_layout)
        vkDestroyDescriptorSetLayout(d.handle, p.descriptor_layout, nullptr);

    p = {};
}
