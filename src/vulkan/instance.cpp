#include "instance.hpp"
#include <cstdio>
#include <cstring>
#include <vector>
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT,
                                                     VkDebugUtilsMessageTypeFlagsEXT,
                                                     const VkDebugUtilsMessengerCallbackDataEXT *d,
                                                     void *) {
    std::fprintf(stderr, "[Vulkan] %s\n", d->pMessage);
    return VK_FALSE;
}
void instance_create(Instance &out, std::span<const char *const> required, bool validation) {
    uint32_t count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&count, nullptr));
    std::vector<VkLayerProperties> layers(count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&count, layers.data()));
    for (auto &l : layers)
        if (std::strcmp(l.layerName, "VK_LAYER_KHRONOS_validation") == 0)
            out.validation = validation;
    if (validation && !out.validation)
        std::fprintf(stderr, "Validation layer unavailable; continuing without it.\n");
    std::vector<const char *> ext(required.begin(), required.end());
    if (out.validation)
        ext.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "Data-oriented Vulkan";
    app.apiVersion = VK_API_VERSION_1_3;
    VkDebugUtilsMessengerCreateInfoEXT debug{
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debug.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug.pfnUserCallback = debug_callback;
    const char *layer = "VK_LAYER_KHRONOS_validation";
    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo = &app;
    ci.enabledExtensionCount = uint32_t(ext.size());
    ci.ppEnabledExtensionNames = ext.data();
    ci.enabledLayerCount = out.validation ? 1 : 0;
    ci.ppEnabledLayerNames = &layer;
    ci.pNext = out.validation ? &debug : nullptr;
    VK_CHECK(vkCreateInstance(&ci, nullptr, &out.handle));
    if (out.validation) {
        auto fn = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(out.handle, "vkCreateDebugUtilsMessengerEXT"));
        if (!fn)
            throw std::runtime_error("Debug messenger function unavailable");
        VK_CHECK(fn(out.handle, &debug, nullptr, &out.debug));
    }
}
void instance_destroy(Instance &i) {
    if (i.debug) {
        auto fn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(i.handle, "vkDestroyDebugUtilsMessengerEXT"));
        if (fn)
            fn(i.handle, i.debug, nullptr);
    }
    if (i.handle)
        vkDestroyInstance(i.handle, nullptr);
    i = {};
}
