#include "pipelines.hpp"
#include "shaders.hpp"

void pipeline_create(const Device &d, Pipeline &p, const std::filesystem::path &directory) {
    VkShaderModule shader{};

    try {
        // Set 0 will contain one storage image at binding 0.
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo descriptors{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        descriptors.bindingCount = 1;
        descriptors.pBindings = &binding;

        VK_CHECK(
            vkCreateDescriptorSetLayout(d.handle, &descriptors, nullptr, &p.descriptor_layout));

        VkPushConstantRange range{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FrameData)};

        VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layout.setLayoutCount = 1;
        layout.pSetLayouts = &p.descriptor_layout;
        layout.pushConstantRangeCount = 1;
        layout.pPushConstantRanges = &range;

        VK_CHECK(vkCreatePipelineLayout(d.handle, &layout, nullptr, &p.layout));

        shader = shader_load(d.handle, directory / "raymarch.comp.spv");

        VkComputePipelineCreateInfo ci{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        ci.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ci.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        ci.stage.module = shader;
        ci.stage.pName = "main";
        ci.layout = p.layout;

        VK_CHECK(vkCreateComputePipelines(d.handle, VK_NULL_HANDLE, 1, &ci, nullptr, &p.handle));
    } catch (...) {
        if (shader)
            vkDestroyShaderModule(d.handle, shader, nullptr);
        pipeline_destroy(d, p);
        throw;
    }

    vkDestroyShaderModule(d.handle, shader, nullptr);
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
