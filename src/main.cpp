// I want to take controller out of here
#include "controller.hpp"
#include "vulkan/vulkan.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <chrono>
#include <cstdio>

int main() {
    GLFWwindow *window = nullptr;
    Vulkan *renderer = nullptr;
    bool glfw_ready = false;

    auto cleanup = [&] {
        vulkan_destroy(renderer);
        if (window)
            glfwDestroyWindow(window);
        if (glfw_ready)
            glfwTerminate();
    };

    Camera camera{};
    Controller controller{};

    try {
        glfwSetErrorCallback([](int code, const char *message) {
            std::fprintf(stderr, "GLFW %d: %s\n", code, message);
        });

        if (!glfwInit())
            throw std::runtime_error("GLFW initialization failed");

        glfw_ready = true;

        if (!glfwVulkanSupported())
            throw std::runtime_error("GLFW cannot find Vulkan support");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(1280, 720, "Raymarcher", nullptr, nullptr);
        if (!window)
            throw std::runtime_error("Window creation failed");

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        uint32_t extension_count = 0;
        const char **extensions = glfwGetRequiredInstanceExtensions(&extension_count);
        if (!extensions)
            throw std::runtime_error("GLFW Vulkan extensions unavailable");

        renderer = vulkan_create({extensions, extension_count}, true);

        VkSurfaceKHR surface{};
        VK_CHECK(glfwCreateWindowSurface(vulkan_instance(*renderer), window, nullptr, &surface));

        int width = 0, height = 0;

        glfwGetFramebufferSize(window, &width, &height);
        vulkan_initialize(*renderer, surface, uint32_t(std::max(width, 1)),
                          uint32_t(std::max(height, 1)), "shaders");

        using Clock = std::chrono::steady_clock;
        auto previous = Clock::now();
        float elapsed_seconds = 0;

        while (!glfwWindowShouldClose(window)) {
            auto now = Clock::now();
            double frame_ms = std::chrono::duration<double, std::milli>(now - previous).count();
            previous = now;

            glfwPollEvents();
            if (glfwWindowShouldClose(window) || glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                break;

            glfwGetFramebufferSize(window, &width, &height);
            if (width == 0 || height == 0) {
                glfwWaitEvents();
                previous = Clock::now();
                continue;
            }

            float dt = float(frame_ms * 0.001);
            controller.camera_rotate(camera, window);
            controller.camera_move(camera, dt, window);
            elapsed_seconds += dt;
            vulkan_draw(*renderer, uint32_t(width), uint32_t(height), elapsed_seconds, camera,
                        float(frame_ms));
        }

        auto stats = vulkan_stats(*renderer);
        std::printf("Rendered %llu frames on %s; CPU %.3f ms, GPU %.3f ms (%s)\n",
                    static_cast<unsigned long long>(stats.submitted_frames), stats.device_name,
                    stats.cpu_work_ms, stats.gpu_ms,
                    stats.gpu_available ? "timestamps" : "unavailable");

        cleanup();

        return 0;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "Fatal: %s\n", e.what());

        cleanup();

        return 1;
    }
}
