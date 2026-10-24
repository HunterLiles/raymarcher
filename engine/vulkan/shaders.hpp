#pragma once

#include "common.hpp"
#include <cstdint>
#include <filesystem>
#include <vector>

VkShaderModule shader_load(VkDevice, const std::filesystem::path &);

std::vector<uint32_t> shader_read(const std::filesystem::path &);
