#include "swapchain.hpp"
#include <algorithm>
void swapchain_create(const Device &d, Swapchain &s, VkSurfaceKHR surface, uint32_t width,
                      uint32_t height) {
    VkSurfaceCapabilitiesKHR caps{};
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(d.physical, surface, &caps));
    if (!(caps.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
        throw std::runtime_error("Surface cannot be a color attachment");
    uint32_t n = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(d.physical, surface, &n, nullptr));
    std::vector<VkSurfaceFormatKHR> formats(n);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(d.physical, surface, &n, formats.data()));
    if (formats.empty())
        throw std::runtime_error("Surface has no formats");
    auto chosen = formats[0];
    if (chosen.format == VK_FORMAT_UNDEFINED)
        chosen = {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    for (auto f : formats)
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            chosen = f;
    s.format = chosen.format;
    s.extent = caps.currentExtent;
    if (s.extent.width == UINT32_MAX)
        s.extent = {std::clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width),
                    std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height)};
    s.min_images = std::max(2u, caps.minImageCount);
    uint32_t count = s.min_images + 1;
    if (caps.maxImageCount) {
        count = std::min(count, caps.maxImageCount);
        if (count < s.min_images)
            throw std::runtime_error("ImGui requires at least two swapchain images");
    }
    VkCompositeAlphaFlagBitsKHR alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    for (auto a : {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                   VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR})
        if (caps.supportedCompositeAlpha & a) {
            alpha = a;
            break;
        }
    uint32_t families[] = {d.queues.graphics_family, d.queues.present_family};
    VkSwapchainCreateInfoKHR ci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    ci.surface = surface;
    ci.minImageCount = count;
    ci.imageFormat = s.format;
    ci.imageColorSpace = chosen.colorSpace;
    ci.imageExtent = s.extent;
    ci.imageArrayLayers = 1;
    ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ci.preTransform = caps.currentTransform;
    ci.compositeAlpha = alpha;
    ci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    ci.clipped = VK_TRUE;
    ci.imageSharingMode =
        families[0] == families[1] ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
    if (ci.imageSharingMode == VK_SHARING_MODE_CONCURRENT) {
        ci.queueFamilyIndexCount = 2;
        ci.pQueueFamilyIndices = families;
    }
    VK_CHECK(vkCreateSwapchainKHR(d.handle, &ci, nullptr, &s.handle));
    VK_CHECK(vkGetSwapchainImagesKHR(d.handle, s.handle, &count, nullptr));
    s.images.resize(count);
    VK_CHECK(vkGetSwapchainImagesKHR(d.handle, s.handle, &count, s.images.data()));
    s.views.resize(count);
    s.present_ready.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
        VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        vi.image = s.images[i];
        vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vi.format = s.format;
        vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VK_CHECK(vkCreateImageView(d.handle, &vi, nullptr, &s.views[i]));
        VkSemaphoreCreateInfo si{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VK_CHECK(vkCreateSemaphore(d.handle, &si, nullptr, &s.present_ready[i]));
    }
}
void swapchain_destroy(const Device &d, Swapchain &s) {
    for (auto sem : s.present_ready)
        if (sem)
            vkDestroySemaphore(d.handle, sem, nullptr);
    for (auto view : s.views)
        if (view)
            vkDestroyImageView(d.handle, view, nullptr);
    if (s.handle)
        vkDestroySwapchainKHR(d.handle, s.handle, nullptr);
    s = {};
}
