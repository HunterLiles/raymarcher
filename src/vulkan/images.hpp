#pragma once
#include "common.hpp"
void image_barrier(VkCommandBuffer, VkImage, VkImageAspectFlags, VkImageLayout, VkImageLayout,
                   VkAccessFlags, VkAccessFlags, VkPipelineStageFlags, VkPipelineStageFlags);
