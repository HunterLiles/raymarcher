#pragma once

#include "engine_math.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

struct GLFWwindow;

namespace engine {
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
    uint32_t shadow_pixel_size{2};

    uint64_t cache_budget{1ull * 1024 * 1024 * 1024};

    bool validation{true};
    bool hot_reload{true};

    std::filesystem::path shader;
    std::filesystem::path sdf;
    std::filesystem::path compiler{"glslc"};
};

class Engine {
  private:
    struct State;
    std::unique_ptr<State> state;

  public:
    explicit Engine(const EngineConfig &);
    ~Engine();

    bool is_running();
    bool draw();
    bool reload_shaders();

    void toggle_fullscreen();
    void set_uniform(std::string_view, float);
    void set_uniform(std::string_view, int32_t);
    void set_uniform(std::string_view, uint32_t);
    void set_uniform(std::string_view, vec2);
    void set_uniform(std::string_view, vec3);
    void set_uniform(std::string_view, vec4);
    void set_uniform(std::string_view, quaternion);

    float dt() const;
    float time() const;
    GLFWwindow *window() const;
    FrameStats stats() const;

    void cache_create(uint32_t id, const CacheConfig &);
    CacheStats cache_stats() const;
};

} // namespace engine
