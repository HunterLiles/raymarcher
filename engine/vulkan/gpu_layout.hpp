#pragma once

#include <array>
#include <cstddef>
#include <type_traits>

struct GpuTypeLayout {
    std::size_t alignment, size;
};

inline constexpr GpuTypeLayout gpu_float{4, 4}, gpu_uint{4, 4};
inline constexpr GpuTypeLayout gpu_vec2{8, 8}, gpu_vec4{16, 16};

struct GpuFieldLayout {
    std::size_t offset, size;
    GpuTypeLayout gpu;
};

constexpr std::size_t align_up(std::size_t n, std::size_t alignment) {
    return (n + alignment - 1) / alignment * alignment;
}

template <class T, std::size_t N>
consteval bool gpu_layout_matches(const std::array<GpuFieldLayout, N> &fields) {
    if (!std::is_standard_layout_v<T> || !std::is_trivially_copyable_v<T>)
        return false;

    std::size_t offset = 0, alignment = 1;

    for (const auto &field : fields) {
        offset = align_up(offset, field.gpu.alignment);

        if (field.offset != offset || field.size != field.gpu.size)
            return false;

        offset += field.gpu.size;
        alignment = alignment > field.gpu.alignment ? alignment : field.gpu.alignment;
    }

    return sizeof(T) == align_up(offset, alignment);
}
