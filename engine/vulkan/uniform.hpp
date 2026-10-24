#pragma once

#include "devices.hpp"
#include <array>
#include <cstddef>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

enum class UniformKind { Float, Int, Uint, Vec2, Vec3, Vec4 };

struct UniformField {
    uint32_t offset{}, size{};
    UniformKind kind{};
};

struct UniformBuffer {
    VkBuffer handle{};
    VkDeviceMemory memory{};
    void *mapped{};
};

struct Uniforms {
    std::map<std::string, UniformField, std::less<>> fields;
    std::vector<std::byte> data;
    std::array<UniformBuffer, 2> buffers{};
};

void uniforms_create(const Device &, Uniforms &, const std::filesystem::path &);
void uniforms_destroy(const Device &, Uniforms &);
void uniforms_set(Uniforms &, std::string_view, const void *, UniformKind);
void uniforms_upload(const Uniforms &, uint32_t slot);
void uniforms_copy(Uniforms &destination, const Uniforms &source);
