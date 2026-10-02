#pragma once
#include "devices.hpp"
struct Commands {
    VkCommandPool pool{};
    VkCommandBuffer buffer{};
};
void commands_create(const Device &, Commands &);
void commands_destroy(const Device &, Commands &);
void commands_begin(VkCommandBuffer);
