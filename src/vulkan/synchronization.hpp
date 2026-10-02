#pragma once
#include "devices.hpp"
struct FrameSync {
    VkSemaphore acquired{};
    VkFence complete{};
};
void sync_create(const Device &, FrameSync &);
void sync_destroy(const Device &, FrameSync &);
