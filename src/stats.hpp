#pragma once
#include <cstdint>
struct FrameStats {
    double cpu_work_ms{}, gpu_ms{};
    bool gpu_available{};
    uint64_t submitted_frames{};
    const char *device_name{};
};
