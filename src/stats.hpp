#pragma once
#include <cstdint>
struct FrameStats {
    double frame_ms{}, cpu_work_ms{}, wait_ms{}, gpu_ms{};
    bool gpu_available{};
    uint64_t submitted_frames{};
    uint32_t draws{}, triangles{}, width{}, height{}, swapchain_images{};
    const char *device_name{};
    bool validation{};
};
