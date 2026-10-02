#pragma once
#include "common.hpp"
#include <filesystem>
VkShaderModule shader_load(VkDevice, const std::filesystem::path &);
