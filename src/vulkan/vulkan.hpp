#pragma once

#include "../camera.hpp"
#include "../stats.hpp"
#include "common.hpp"
#include <filesystem>
#include <span>

struct Vulkan;

Vulkan *vulkan_create(std::span<const char *const> instance_extensions, bool validation);

VkInstance vulkan_instance(const Vulkan &);

void vulkan_initialize(Vulkan &, VkSurfaceKHR, uint32_t width, uint32_t height,
                       const std::filesystem::path &shaders);

void vulkan_destroy(Vulkan *);

bool vulkan_draw(Vulkan &, uint32_t width, uint32_t height, float time, const Camera &,
                 float frame_ms);

FrameStats vulkan_stats(const Vulkan &);
