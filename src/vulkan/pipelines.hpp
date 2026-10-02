#pragma once
#include "devices.hpp"
#include <filesystem>
// Matches the fragment shader's push-constant block.
struct FrameData {
    float width, height, time;
};
static_assert(sizeof(FrameData) == 12);

struct Pipeline {
    VkPipeline handle{};
    VkPipelineLayout layout{};
};

void pipeline_create(const Device &, Pipeline &, VkFormat, const std::filesystem::path &);
void pipeline_destroy(const Device &, Pipeline &);
