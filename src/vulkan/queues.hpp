#pragma once
#include "common.hpp"
#include <cstdint>
struct Queues {
    uint32_t graphics_family{UINT32_MAX}, present_family{UINT32_MAX}, timestamp_bits{};
    VkQueue graphics{}, present{};
};
bool queues_find(VkPhysicalDevice, VkSurfaceKHR, Queues &);
void queues_load(VkDevice, Queues &);
