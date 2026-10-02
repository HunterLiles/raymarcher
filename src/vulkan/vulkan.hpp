#pragma once
#include "../stats.hpp"
#include "common.hpp"
#include <filesystem>
#include <span>
struct Vulkan;
Vulkan *vulkan_create(std::span<const char *const> instance_extensions, bool validation);
VkInstance vulkan_instance(const Vulkan &);
// Takes surface ownership immediately, including on initialization failure.
void vulkan_initialize(Vulkan &, VkSurfaceKHR, uint32_t width, uint32_t height,
                       const std::filesystem::path &shaders);
void vulkan_destroy(Vulkan *);
// Returns false when no frame was submitted (zero extent or an out-of-date swapchain).
bool vulkan_draw(Vulkan &, uint32_t width, uint32_t height, float time);
FrameStats vulkan_stats(const Vulkan &);
