#pragma once
#include <stdexcept>
#include <string>
#include <vulkan/vulkan.h>
inline void vk_check(VkResult r, const char *call) {
    if (r != VK_SUCCESS)
        throw std::runtime_error(std::string(call) + " failed (VkResult " + std::to_string(r) +
                                 ")");
}
#define VK_CHECK(call) vk_check((call), #call)
