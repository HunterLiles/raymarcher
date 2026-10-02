#pragma once
#include "queues.hpp"
struct Device {
    VkPhysicalDevice physical{};
    VkDevice handle{};
    VkPhysicalDeviceProperties properties{};
    VkPhysicalDeviceMemoryProperties memory{};
    Queues queues;
};
void device_create(Device &, VkInstance, VkSurfaceKHR);
void device_destroy(Device &);
uint32_t memory_type(const Device &, uint32_t mask, VkMemoryPropertyFlags);
