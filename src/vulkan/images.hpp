#pragma once

#include "common.hpp"
#include "devices.hpp"

struct RenderImage {
    VkImage handle{};
    VkDeviceMemory memory{};
    VkImageView view{};

    bool initialized{};
};

void image_barrier(VkCommandBuffer, VkImage, VkImageAspectFlags, VkImageLayout, VkImageLayout,
                   VkAccessFlags, VkAccessFlags, VkPipelineStageFlags, VkPipelineStageFlags);

void image_create(const Device &d, RenderImage &image);
void image_destroy(const Device &d, RenderImage &image);

uint32_t memory_type(VkPhysicalDevice physical, uint32_t allowed_types,
                     VkMemoryPropertyFlags required);
