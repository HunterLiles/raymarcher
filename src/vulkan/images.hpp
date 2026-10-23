#pragma once

#include "common.hpp"
#include "devices.hpp"

struct RenderImage {
    VkImage handle{};
    VkDeviceMemory memory{};
    VkImageView view{};
    VkExtent2D extent{};

    bool initialized{};
};

inline uint32_t pixel_size = 3;

inline VkExtent2D render_extent(uint32_t window_width, uint32_t window_height) {
    return {(window_width + pixel_size - 1) / pixel_size,
            (window_height + pixel_size - 1) / pixel_size};
}

void image_barrier(VkCommandBuffer, VkImage, VkImageAspectFlags, VkImageLayout, VkImageLayout,
                   VkAccessFlags, VkAccessFlags, VkPipelineStageFlags, VkPipelineStageFlags);

void image_create(const Device &d, RenderImage &image, VkExtent2D extent);
void image_destroy(const Device &d, RenderImage &image);

uint32_t memory_type(VkPhysicalDevice physical, uint32_t allowed_types,
                     VkMemoryPropertyFlags required);
