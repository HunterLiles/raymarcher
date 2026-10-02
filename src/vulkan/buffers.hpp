#pragma once
#include "devices.hpp"
struct Buffer {
    VkBuffer handle{};
    VkDeviceMemory memory{};
    VkDeviceSize size{};
    VkMemoryPropertyFlags properties{};
};
void buffer_create(const Device &, Buffer &, VkDeviceSize, VkBufferUsageFlags,
                   VkMemoryPropertyFlags);
void buffer_write(const Device &, const Buffer &, const void *, VkDeviceSize);
void buffer_destroy(const Device &, Buffer &);
