#pragma once
#include "devices.hpp"
#include <vector>
struct Swapchain {
    VkSwapchainKHR handle{};
    VkFormat format{};
    VkExtent2D extent{};
    uint32_t min_images{};
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
    // Presentation completion is tied to reacquiring an image, not a frame fence.
    std::vector<VkSemaphore> present_ready;
};
void swapchain_create(const Device &, Swapchain &, VkSurfaceKHR, uint32_t width, uint32_t height);
void swapchain_destroy(const Device &, Swapchain &);
