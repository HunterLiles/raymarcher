#pragma once

#include "../engine_math.hpp"
#include "devices.hpp"
#include "gpu_layout.hpp"
#include <cstddef>
#include <filesystem>

struct FrameData {
#define GPU_FIELD(glsl, cpp, name) alignas(gpu_##glsl.alignment) cpp name{};
#include "../frame_fields.inc"
#undef GPU_FIELD
};

static_assert(gpu_layout_matches<FrameData>(std::array{
#define GPU_FIELD(glsl, cpp, name)                                                                 \
    GpuFieldLayout{offsetof(FrameData, name), sizeof(cpp), gpu_##glsl},
#include "../frame_fields.inc"
#undef GPU_FIELD
}));

using CacheDispatch = FrameData;

struct Pipeline {
    VkPipeline handle{};
    VkPipelineLayout layout{};
    VkDescriptorSetLayout descriptor_layout{};
};

VkPipeline compute_pipeline_create(const Device &, VkPipelineLayout, const std::filesystem::path &);

void pipeline_create(const Device &, Pipeline &, const std::filesystem::path &);

void pipeline_destroy(const Device &, Pipeline &);
