#include "synchronization.hpp"
void sync_create(const Device &d, FrameSync &s) {
    VkSemaphoreCreateInfo si{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VK_CHECK(vkCreateSemaphore(d.handle, &si, nullptr, &s.acquired));
    VkFenceCreateInfo fi{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHECK(vkCreateFence(d.handle, &fi, nullptr, &s.complete));
}
void sync_destroy(const Device &d, FrameSync &s) {
    if (s.acquired)
        vkDestroySemaphore(d.handle, s.acquired, nullptr);
    if (s.complete)
        vkDestroyFence(d.handle, s.complete, nullptr);
    s = {};
}
