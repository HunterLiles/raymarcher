#pragma once
#include "common.hpp"
#include <span>
struct Instance {
    VkInstance handle{};
    VkDebugUtilsMessengerEXT debug{};
    bool validation{};
};
void instance_create(Instance &, std::span<const char *const> extensions, bool validation);
void instance_destroy(Instance &);
