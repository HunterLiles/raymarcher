#include "engine.hpp"
#include "hot_reload.hpp"
#include "vulkan/vulkan.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <stdexcept>

namespace engine {

using Clock = std::chrono::steady_clock;

struct Engine::State {
    GLFWwindow *window{};
    int window_x{}, window_y{};
    int window_width{}, window_height{};
    Vulkan *renderer{};
    bool glfw_ready{};
    bool hot_reload{};
    HotReload shaders;

    int width{}, height{};
    float frame_ms{}, dt{}, time{};
    Clock::time_point previous;

    ~State() {
        vulkan_destroy(renderer);

        if (window)
            glfwDestroyWindow(window);

        if (glfw_ready)
            glfwTerminate();
    }
};

Engine::Engine(const EngineConfig &config) : state(std::make_unique<State>()) {
    auto &s = *state;

    if (!config.pixel_size || !config.shadow_pixel_size)
        throw std::invalid_argument("Pixel size must be greater than zero");

    if (config.shader.empty())
        throw std::invalid_argument("A compute shader is required");

    ShaderBuild build;
    auto binary = config.shader;

    if (config.shader.extension() == ".glsl") {
        if (config.sdf.empty())
            throw std::invalid_argument("An SDF source file is required for shader caches");

        s.shaders.configure(config.shader, config.sdf, config.compiler);
        build = s.shaders.build();
        binary = build.render;
        s.hot_reload = config.hot_reload;
    }

    glfwSetErrorCallback([](int code, const char *message) {
        std::fprintf(stderr, "GLFW %d: %s\n", code, message);
    });

    if (!glfwInit())
        throw std::runtime_error("GLFW initialization failed");

    s.glfw_ready = true;

    if (!glfwVulkanSupported())
        throw std::runtime_error("GLFW cannot find Vulkan support");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    s.window = glfwCreateWindow(config.width, config.height, config.title, nullptr, nullptr);

    if (!s.window)
        throw std::runtime_error("Window creation failed");

    glfwSetWindowUserPointer(s.window, this);
    glfwSetKeyCallback(s.window, [](GLFWwindow *window, int key, int, int action, int mods) {
        if (action == GLFW_PRESS && (mods & GLFW_MOD_ALT) &&
            (key == GLFW_KEY_ENTER || key == GLFW_KEY_F)) {
            auto *engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));
            engine->toggle_fullscreen();
        }
    });

    uint32_t count{};
    const char **extensions = glfwGetRequiredInstanceExtensions(&count);

    if (!extensions)
        throw std::runtime_error("GLFW Vulkan extensions unavailable");

    s.renderer = vulkan_create({extensions, count}, config.validation);
    vulkan_cache_budget(*s.renderer, config.cache_budget);

    VkSurfaceKHR surface{};
    VK_CHECK(glfwCreateWindowSurface(vulkan_instance(*s.renderer), s.window, nullptr, &surface));

    glfwGetFramebufferSize(s.window, &s.width, &s.height);

    vulkan_initialize(*s.renderer, surface, uint32_t(std::max(s.width, 1)),
                      uint32_t(std::max(s.height, 1)), binary, config.pixel_size,
                      config.shadow_pixel_size, s.shaders.enabled() ? &build : nullptr);

    s.previous = Clock::now();
}

Engine::~Engine() = default;

bool Engine::is_running() {
    auto &s = *state;

    for (;;) {
        glfwPollEvents();

        if (glfwWindowShouldClose(s.window))
            return false;

        glfwGetFramebufferSize(s.window, &s.width, &s.height);

        if (!s.width || !s.height) {
            glfwWaitEvents();
            s.previous = Clock::now();
            continue;
        }

        if (s.hot_reload && s.shaders.changed()) {
            reload_shaders();
            s.previous = Clock::now();
            s.frame_ms = 0.0f;
        }

        auto now = Clock::now();
        s.frame_ms = std::chrono::duration<float, std::milli>(now - s.previous).count();
        s.previous = now;

        s.dt = s.frame_ms * 0.001f;
        s.time += s.dt;
        return true;
    }
}

bool Engine::draw() {
    auto &s = *state;

    return vulkan_draw(*s.renderer, uint32_t(s.width), uint32_t(s.height), s.time);
}

#define UNIFORM_SETTER(type, kind)                                                                 \
    void Engine::set_uniform(std::string_view name, type value) {                                  \
        vulkan_set_uniform(*state->renderer, name, &value, UniformKind::kind);                     \
    }

UNIFORM_SETTER(float, Float);
UNIFORM_SETTER(int32_t, Int);
UNIFORM_SETTER(uint32_t, Uint);
UNIFORM_SETTER(vec2, Vec2);
UNIFORM_SETTER(vec3, Vec3);
UNIFORM_SETTER(vec4, Vec4);
UNIFORM_SETTER(quaternion, Vec4);
#undef UNIFORM_SETTER

float Engine::dt() const {
    return state->dt;
}
float Engine::time() const {
    return state->time;
}
GLFWwindow *Engine::window() const {
    return state->window;
}
FrameStats Engine::stats() const {
    return vulkan_stats(*state->renderer);
}

void Engine::cache_create(uint32_t id, const CacheConfig &config) {
    vulkan_cache_create(*state->renderer, id, config.shader, config.entries,
                        config.updates_per_second);
}

CacheStats Engine::cache_stats() const {
    return vulkan_cache_stats(*state->renderer);
}

bool Engine::reload_shaders() {
    auto &s = *state;

    if (!s.shaders.enabled())
        return false;

    try {
        auto build = s.shaders.build();
        vulkan_reload(*s.renderer, build);

        s.previous = Clock::now();
        std::fprintf(stderr, "Shaders reloaded\n");
        return true;
    } catch (const std::exception &error) {
        s.previous = Clock::now();
        std::fprintf(stderr, "Shader reload failed; keeping current scene:\n%s\n", error.what());
        return false;
    }
}

void Engine::toggle_fullscreen() {
    auto &s = *state;

    if (glfwGetWindowMonitor(s.window)) {
        glfwSetWindowMonitor(s.window, nullptr, s.window_x, s.window_y, s.window_width,
                             s.window_height, GLFW_DONT_CARE);
        return;
    }

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    if (!monitor)
        return;

    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    if (!mode)
        return;

    glfwGetWindowSize(s.window, &s.window_width, &s.window_height);

    if (glfwGetPlatform() != GLFW_PLATFORM_WAYLAND)
        glfwGetWindowPos(s.window, &s.window_x, &s.window_y);

    glfwSetWindowMonitor(s.window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
}

} // namespace engine
