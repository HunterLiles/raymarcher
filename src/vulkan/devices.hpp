#pragma once
#include "queues.hpp"
struct Device {
    VkPhysicalDevice physical{};
    VkDevice handle{};
    VkPhysicalDeviceProperties properties{};
    Queues queues;
};
void device_create(Device &, VkInstance, VkSurfaceKHR);
void device_destroy(Device &);
