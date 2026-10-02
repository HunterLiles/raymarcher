#pragma once
#include "../render_data.hpp"
#include "devices.hpp"
#include <filesystem>
struct Pipeline {
    VkPipeline handle{};
    VkPipelineLayout layout{};
};

void pipeline_create(const Device &, Pipeline &, VkFormat, const std::filesystem::path &);
void pipeline_destroy(const Device &, Pipeline &);
