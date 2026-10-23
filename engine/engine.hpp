#pragma once

#include "engine_math.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>

struct GLFWwindow;

namespace engine {
struct Camera {
    vec3 pos{0.0f, 2.0f, -5.0f};
    quaternion orientation{};
};

struct FrameStats {
    double cpu_work_ms{}, gpu_ms{};
    bool gpu_available{};
    uint64_t submitted_frames{};
    const char *device_name{};
};

struct CacheConfig {
    std::filesystem::path shader;
    uint32_t entries{};
    float updates_per_second{};
};

struct CacheStats {
    uint64_t used{}, allocated{}, limit{};
};

struct EngineConfig {
    int width{1280}, height{720};
    const char *title{"Vulkan Engine"};
    uint32_t pixel_size{2};
    std::filesystem::path shader;
    bool validation{true};
    std::filesystem::path sdf;
    std::filesystem::path compiler{"glslc"};
    bool hot_reload{true};
    uint64_t cache_budget{1ull * 1024 * 1024 * 1024};
};

class Engine {
  private:
    struct State;
    std::unique_ptr<State> state;

  public:
    explicit Engine(const EngineConfig &);
    ~Engine();

    bool is_running();
    bool draw(const Camera &);
    bool reload_shaders();
    void toggle_fullscreen();

    float dt() const;
    float time() const;
    GLFWwindow *window() const;
    FrameStats stats() const;

    void cache_create(uint32_t id, const CacheConfig &);
    CacheStats cache_stats() const;
};

} // namespace engine
