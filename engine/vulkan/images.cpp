#include "images.hpp"

void image_barrier(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspect,
                   VkImageLayout old_layout, VkImageLayout new_layout, VkAccessFlags src,
                   VkAccessFlags dst, VkPipelineStageFlags src_stage,
                   VkPipelineStageFlags dst_stage) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    b.srcAccessMask = src;
    b.dstAccessMask = dst;

    b.oldLayout = old_layout;
    b.newLayout = new_layout;

    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    b.image = image;
    b.subresourceRange = {aspect, 0, 1, 0, 1};

    vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &b);
}

void image_create(const Device &d, RenderImage &image, VkExtent2D extent) {
    constexpr VkFormat render_format = VK_FORMAT_R8G8B8A8_UNORM;

    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image.extent = extent;

    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = render_format;
    ci.extent = {extent.width, extent.height, 1};
    ci.mipLevels = 1;
    ci.arrayLayers = 1;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    constexpr VkFormatFeatureFlags required =
        VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT;

    VkFormatProperties props{};
    vkGetPhysicalDeviceFormatProperties(d.physical, render_format, &props);

    if ((props.optimalTilingFeatures & required) != required)
        throw std::runtime_error("Render format cannot support storage writes and blits");

    VK_CHECK(vkCreateImage(d.handle, &ci, nullptr, &image.handle));

    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(d.handle, image.handle, &requirements);

    VkMemoryDedicatedAllocateInfo dedicated{VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
    dedicated.image = image.handle;

    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.pNext = &dedicated;
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex =
        memory_type(d.physical, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_CHECK(vkAllocateMemory(d.handle, &allocation, nullptr, &image.memory));

    VK_CHECK(vkBindImageMemory(d.handle, image.handle, image.memory, 0));

    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view.image = image.handle;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = render_format;
    view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    VK_CHECK(vkCreateImageView(d.handle, &view, nullptr, &image.view));
}

void image_destroy(const Device &d, RenderImage &image) {
    if (image.view)
        vkDestroyImageView(d.handle, image.view, nullptr);
    if (image.handle)
        vkDestroyImage(d.handle, image.handle, nullptr);
    if (image.memory)
        vkFreeMemory(d.handle, image.memory, nullptr);

    image = {};
}

uint32_t memory_type(VkPhysicalDevice physical, uint32_t allowed_types,
                     VkMemoryPropertyFlags required) {
    VkPhysicalDeviceMemoryProperties props{};
    vkGetPhysicalDeviceMemoryProperties(physical, &props);

    for (uint32_t i = 0; i < props.memoryTypeCount; ++i) {
        bool allowed = (allowed_types & (1u << i)) != 0;
        bool suitable = (props.memoryTypes[i].propertyFlags & required) == required;

        if (allowed && suitable)
            return i;
    }

    throw std::runtime_error("No suitable image memory type");
}
