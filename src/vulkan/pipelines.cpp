#include "pipelines.hpp"
#include "shaders.hpp"
void pipeline_create(const Device &d, Pipeline &p, VkFormat format,
                     const std::filesystem::path &directory) {
    VkShaderModule vs{}, ps{};
    try {
        vs = shader_load(d.handle, directory / "fullscreen.vert.spv");
        ps = shader_load(d.handle, directory / "raymarch.frag.spv");
        VkPushConstantRange range{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(FrameData)};
        VkPipelineLayoutCreateInfo li{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        li.pushConstantRangeCount = 1;
        li.pPushConstantRanges = &range;
        VK_CHECK(vkCreatePipelineLayout(d.handle, &li, nullptr, &p.layout));
        VkPipelineShaderStageCreateInfo stages[2]{};
        for (auto &s : stages)
            s.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vs;
        stages[0].pName = "main";
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = ps;
        stages[1].pName = "main";
        // The vertex shader generates a full-screen triangle from gl_VertexIndex.
        VkPipelineVertexInputStateCreateInfo vertex{
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo assembly{
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport.viewportCount = 1;
        viewport.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo raster{
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = VK_CULL_MODE_NONE;
        raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        raster.lineWidth = 1;
        VkPipelineMultisampleStateCreateInfo ms{
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo depth{
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depth.depthTestEnable = VK_FALSE;
        depth.depthWriteEnable = VK_FALSE;
        VkPipelineColorBlendAttachmentState attachment{};
        attachment.colorWriteMask = 0xf;
        VkPipelineColorBlendStateCreateInfo blend{
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blend.attachmentCount = 1;
        blend.pAttachments = &attachment;
        VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount = 2;
        dynamic.pDynamicStates = dynamic_states;
        VkPipelineRenderingCreateInfo rendering{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        rendering.colorAttachmentCount = 1;
        rendering.pColorAttachmentFormats = &format;
        VkGraphicsPipelineCreateInfo ci{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        ci.pNext = &rendering;
        ci.stageCount = 2;
        ci.pStages = stages;
        ci.pVertexInputState = &vertex;
        ci.pInputAssemblyState = &assembly;
        ci.pViewportState = &viewport;
        ci.pRasterizationState = &raster;
        ci.pMultisampleState = &ms;
        ci.pDepthStencilState = &depth;
        ci.pColorBlendState = &blend;
        ci.pDynamicState = &dynamic;
        ci.layout = p.layout;
        VK_CHECK(vkCreateGraphicsPipelines(d.handle, VK_NULL_HANDLE, 1, &ci, nullptr, &p.handle));
    } catch (...) {
        if (vs)
            vkDestroyShaderModule(d.handle, vs, nullptr);
        if (ps)
            vkDestroyShaderModule(d.handle, ps, nullptr);
        throw;
    }
    vkDestroyShaderModule(d.handle, vs, nullptr);
    vkDestroyShaderModule(d.handle, ps, nullptr);
}
void pipeline_destroy(const Device &d, Pipeline &p) {
    if (p.handle)
        vkDestroyPipeline(d.handle, p.handle, nullptr);
    if (p.layout)
        vkDestroyPipelineLayout(d.handle, p.layout, nullptr);
    p = {};
}
