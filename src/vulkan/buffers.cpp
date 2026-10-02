#include "buffers.hpp"
#include <cstring>
void buffer_create(const Device &d, Buffer &b, VkDeviceSize size, VkBufferUsageFlags usage,
                   VkMemoryPropertyFlags flags) {
    b.size = size;
    VkBufferCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    ci.size = size;
    ci.usage = usage;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateBuffer(d.handle, &ci, nullptr, &b.handle));
    VkMemoryRequirements req{};
    vkGetBufferMemoryRequirements(d.handle, b.handle, &req);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = memory_type(d, req.memoryTypeBits, flags);
    b.properties = d.memory.memoryTypes[ai.memoryTypeIndex].propertyFlags;
    VK_CHECK(vkAllocateMemory(d.handle, &ai, nullptr, &b.memory));
    VK_CHECK(vkBindBufferMemory(d.handle, b.handle, b.memory, 0));
}
void buffer_write(const Device &d, const Buffer &b, const void *data, VkDeviceSize bytes) {
    if (bytes > b.size)
        throw std::runtime_error("Buffer write exceeds allocation");
    if (!(b.properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
        throw std::runtime_error("buffer_write requires host-visible memory");
    if (!bytes)
        return;
    void *p{};
    VK_CHECK(vkMapMemory(d.handle, b.memory, 0, VK_WHOLE_SIZE, 0, &p));
    std::memcpy(p, data, size_t(bytes));
    VkResult result = VK_SUCCESS;
    if (!(b.properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
        range.memory = b.memory;
        range.size = VK_WHOLE_SIZE;
        result = vkFlushMappedMemoryRanges(d.handle, 1, &range);
    }
    vkUnmapMemory(d.handle, b.memory);
    VK_CHECK(result);
}
void buffer_destroy(const Device &d, Buffer &b) {
    if (b.handle)
        vkDestroyBuffer(d.handle, b.handle, nullptr);
    if (b.memory)
        vkFreeMemory(d.handle, b.memory, nullptr);
    b = {};
}
