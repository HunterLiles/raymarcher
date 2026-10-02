#pragma once
#include "../render_data.hpp"
#include "../stats.hpp"
#include "common.hpp"
#include <filesystem>
#include <span>
struct Vulkan;
struct GuiDeviceInfo {
    VkInstance instance{};
    VkPhysicalDevice physical{};
    VkDevice device{};
    VkQueue queue{};
    uint32_t queue_family{};
    VkFormat color_format{};
    uint32_t min_images{}, image_count{}, generation{};
};
Vulkan *vulkan_create(std::span<const char *const> instance_extensions, bool validation);
VkInstance vulkan_instance(const Vulkan &);
// Takes surface ownership immediately, including on initialization failure.
void vulkan_initialize(Vulkan &, VkSurfaceKHR, uint32_t width, uint32_t height,
                       const std::filesystem::path &shaders);
void vulkan_destroy(Vulkan *);
void vulkan_wait_idle(Vulkan &);
bool vulkan_begin(Vulkan &, uint32_t width, uint32_t height);
using OverlayRecorder = void (*)(VkCommandBuffer, void *);
void vulkan_draw(Vulkan &, const RenderData &, OverlayRecorder, void *overlay, double frame_ms,
                 double app_cpu_ms);
GuiDeviceInfo vulkan_gui_info(const Vulkan &);
FrameStats vulkan_stats(const Vulkan &);
float vulkan_aspect(const Vulkan &);
