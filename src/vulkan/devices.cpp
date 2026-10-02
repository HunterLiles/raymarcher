#include "devices.hpp"
#include <cstring>
#include <vector>
void device_create(Device &d, VkInstance instance, VkSurfaceKHR surface) {
    uint32_t n = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &n, nullptr));
    std::vector<VkPhysicalDevice> devices(n);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &n, devices.data()));
    int best = -1;
    for (auto p : devices) {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(p, &props);
        if (props.apiVersion < VK_API_VERSION_1_3)
            continue;
        VkPhysicalDeviceVulkan13Features f13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        VkPhysicalDeviceFeatures2 f{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        f.pNext = &f13;
        vkGetPhysicalDeviceFeatures2(p, &f);
        if (!f13.dynamicRendering)
            continue;
        Queues q{};
        if (!queues_find(p, surface, q))
            continue;
        uint32_t ext_n = 0;
        VK_CHECK(vkEnumerateDeviceExtensionProperties(p, nullptr, &ext_n, nullptr));
        std::vector<VkExtensionProperties> extensions(ext_n);
        VK_CHECK(vkEnumerateDeviceExtensionProperties(p, nullptr, &ext_n, extensions.data()));
        bool swap = false;
        for (auto &e : extensions)
            swap |= std::strcmp(e.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
        if (!swap)
            continue;
        uint32_t formats = 0, modes = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(p, surface, &formats, nullptr));
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(p, surface, &modes, nullptr));
        if (!formats || !modes)
            continue;
        int score = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 100 : 10;
        if (score > best) {
            best = score;
            d.physical = p;
            d.properties = props;
            d.queues = q;
        }
    }
    if (!d.physical)
        throw std::runtime_error(
            "No Vulkan 1.3 device with dynamic rendering, presentation support");
    float priority = 1;
    VkDeviceQueueCreateInfo qi[2]{};
    uint32_t families[] = {d.queues.graphics_family, d.queues.present_family};
    uint32_t count = families[0] == families[1] ? 1 : 2;
    for (uint32_t i = 0; i < count; ++i) {
        qi[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qi[i].queueFamilyIndex = families[i];
        qi[i].queueCount = 1;
        qi[i].pQueuePriorities = &priority;
    }
    VkPhysicalDeviceVulkan13Features features{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features.dynamicRendering = VK_TRUE;
    const char *extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    VkDeviceCreateInfo ci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    ci.pNext = &features;
    ci.queueCreateInfoCount = count;
    ci.pQueueCreateInfos = qi;
    ci.enabledExtensionCount = 1;
    ci.ppEnabledExtensionNames = &extension;
    VK_CHECK(vkCreateDevice(d.physical, &ci, nullptr, &d.handle));
    queues_load(d.handle, d.queues);
    vkGetPhysicalDeviceMemoryProperties(d.physical, &d.memory);
}
void device_destroy(Device &d) {
    if (d.handle)
        vkDestroyDevice(d.handle, nullptr);
    d = {};
}
uint32_t memory_type(const Device &d, uint32_t mask, VkMemoryPropertyFlags flags) {
    for (uint32_t i = 0; i < d.memory.memoryTypeCount; ++i)
        if ((mask & (1u << i)) && (d.memory.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    throw std::runtime_error("No compatible Vulkan memory type");
}
