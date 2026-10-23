#pragma once

#include "../engine_math.hpp"
#include "devices.hpp"
#include <cstddef>
#include <filesystem>

struct alignas(16) FrameData {
    quaternion camera_rot;
    vec4 camera_pos;
    vec2 resolution;
    float time;
};

static_assert(offsetof(FrameData, camera_rot) == 0);
static_assert(offsetof(FrameData, camera_pos) == 16);
static_assert(offsetof(FrameData, resolution) == 32);
static_assert(offsetof(FrameData, time) == 40);
static_assert(sizeof(FrameData) == 48);

struct Pipeline {
    VkPipeline handle{};
    VkPipelineLayout layout{};
    VkDescriptorSetLayout descriptor_layout{};
};

void pipeline_create(const Device &, Pipeline &, const std::filesystem::path &);

void pipeline_destroy(const Device &, Pipeline &);
