#pragma once

#include "../engine.hpp"
#include "common.hpp"
#include <filesystem>
#include <span>

struct Vulkan;
struct ShaderBuild;

Vulkan *vulkan_create(std::span<const char *const> instance_extensions, bool validation);

VkInstance vulkan_instance(const Vulkan &);

void vulkan_initialize(Vulkan &, VkSurfaceKHR, uint32_t width, uint32_t height,
                       const std::filesystem::path &shader, uint32_t pixel_size,
                       const ShaderBuild *build = nullptr);

void vulkan_reload(Vulkan &, const ShaderBuild &);

void vulkan_destroy(Vulkan *);

bool vulkan_draw(Vulkan &, uint32_t width, uint32_t height, float time, const engine::Camera &,
                 float frame_ms);

engine::FrameStats vulkan_stats(const Vulkan &);

void vulkan_cache_create(Vulkan &, uint32_t id, const std::filesystem::path &shader, uint32_t count,
                         float fps);

void vulkan_cache_budget(Vulkan &, VkDeviceSize bytes);

engine::CacheStats vulkan_cache_stats(const Vulkan &);
