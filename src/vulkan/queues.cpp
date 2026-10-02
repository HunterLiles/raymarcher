#include "queues.hpp"
#include <vector>
bool queues_find(VkPhysicalDevice p, VkSurfaceKHR surface, Queues &q) {
    uint32_t n = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(p, &n, nullptr);
    std::vector<VkQueueFamilyProperties> families(n);
    vkGetPhysicalDeviceQueueFamilyProperties(p, &n, families.data());
    for (uint32_t i = 0; i < n; ++i) {
        VkBool32 present = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(p, i, surface, &present));
        if (families[i].queueCount && (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            if (q.graphics_family == UINT32_MAX || present) {
                q.graphics_family = i;
                q.timestamp_bits = families[i].timestampValidBits;
            }
            if (present) {
                q.present_family = i;
                break;
            }
        }
        if (present)
            q.present_family = i;
    }
    return q.graphics_family != UINT32_MAX && q.present_family != UINT32_MAX;
}
void queues_load(VkDevice d, Queues &q) {
    vkGetDeviceQueue(d, q.graphics_family, 0, &q.graphics);
    vkGetDeviceQueue(d, q.present_family, 0, &q.present);
}
