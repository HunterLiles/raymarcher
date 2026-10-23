#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

struct CacheSpec {
    uint32_t id{}, count{};
    float fps{};
};

struct ShaderBuild {
    std::filesystem::path render, cache;
    std::vector<CacheSpec> caches;
};

class HotReload {
    using Clock = std::chrono::steady_clock;
    using Files =
        std::map<std::filesystem::path, std::pair<std::filesystem::file_time_type, uintmax_t>>;

    std::filesystem::path source, sdf, compiler, work;
    Files observed, attempted;
    Clock::time_point next_check{};

    Files scan() const;
    void run(std::vector<std::string>) const;

  public:
    HotReload() = default;
    HotReload(const HotReload &) = delete;
    HotReload &operator=(const HotReload &) = delete;
    ~HotReload();

    void configure(const std::filesystem::path &, const std::filesystem::path &,
                   const std::filesystem::path &);

    bool enabled() const {
        return !source.empty();
    }
    bool changed();
    ShaderBuild build();
};
