#include "uniform.hpp"
#include "images.hpp"
#include "shaders.hpp"
#include <algorithm>
#include <cstring>
#include <spirv_reflect.h>

static UniformKind uniform_kind(const SpvReflectBlockVariable &field) {
    auto flags = field.type_description->type_flags;
    const auto &n = field.numeric;

    if (field.array.dims_count || field.member_count || n.matrix.column_count ||
        n.scalar.width != 32)
        throw std::runtime_error("Unsupported uniform type: " + std::string(field.name));

    if (flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
        switch (n.vector.component_count) {
        case 0:
        case 1:
            return UniformKind::Float;
        case 2:
            return UniformKind::Vec2;
        case 3:
            return UniformKind::Vec3;
        case 4:
            return UniformKind::Vec4;
        }
    }

    if ((flags & SPV_REFLECT_TYPE_FLAG_INT) && n.vector.component_count <= 1)
        return n.scalar.signedness ? UniformKind::Int : UniformKind::Uint;

    throw std::runtime_error("Unsupported uniform type: " + std::string(field.name));
}

void uniforms_create(const Device &d, Uniforms &u, const std::filesystem::path &path) {
    const auto code = shader_read(path);
    spv_reflect::ShaderModule module(code);
    if (module.GetResult() != SPV_REFLECT_RESULT_SUCCESS)
        throw std::runtime_error("Cannot reflect shader: " + path.string());

    const auto &reflection = module.GetShaderModule();
    uint32_t bytes = 16;

    for (uint32_t i = 0; i < reflection.descriptor_binding_count; ++i) {
        const auto &binding = reflection.descriptor_bindings[i];
        if (binding.set != 0 || binding.binding != 2)
            continue;

        if (binding.descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
            binding.count != 1)
            throw std::runtime_error("Set 0, binding 2 must be one uniform buffer");

        bytes = std::max(bytes, binding.block.padded_size);
        for (uint32_t j = 0; j < binding.block.member_count; ++j) {
            const auto &field = binding.block.members[j];
            if (!field.name || !*field.name)
                throw std::runtime_error("Uniform names missing; compile without stripping names");

            auto kind = uniform_kind(field);
            uint32_t size = 4 * std::max(1u, field.numeric.vector.component_count);
            u.fields.emplace(field.name, UniformField{field.offset, size, kind});
        }
    }

    if (bytes > d.properties.limits.maxUniformBufferRange)
        throw std::runtime_error("Uniform block exceeds device limit");

    u.data.resize(bytes);
    for (auto &buffer : u.buffers) {
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        info.size = bytes;
        info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_CHECK(vkCreateBuffer(d.handle, &info, nullptr, &buffer.handle));

        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(d.handle, buffer.handle, &requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex =
            memory_type(d.physical, requirements.memoryTypeBits,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        VK_CHECK(vkAllocateMemory(d.handle, &allocation, nullptr, &buffer.memory));
        VK_CHECK(vkBindBufferMemory(d.handle, buffer.handle, buffer.memory, 0));
        VK_CHECK(vkMapMemory(d.handle, buffer.memory, 0, bytes, 0, &buffer.mapped));
    }
}

void uniforms_destroy(const Device &d, Uniforms &u) {
    for (auto &buffer : u.buffers) {
        if (buffer.mapped)
            vkUnmapMemory(d.handle, buffer.memory);
        if (buffer.handle)
            vkDestroyBuffer(d.handle, buffer.handle, nullptr);
        if (buffer.memory)
            vkFreeMemory(d.handle, buffer.memory, nullptr);
    }
    u = {};
}

void uniforms_set(Uniforms &u, std::string_view name, const void *value, UniformKind kind) {
    auto it = u.fields.find(name);
    if (it == u.fields.end())
        throw std::runtime_error("Unknown uniform: " + std::string(name));

    const auto &field = it->second;
    if (field.kind != kind)
        throw std::runtime_error("Uniform type mismatch: " + std::string(name));

    std::memcpy(u.data.data() + field.offset, value, field.size);
}

void uniforms_upload(const Uniforms &u, uint32_t slot) {
    std::memcpy(u.buffers[slot].mapped, u.data.data(), u.data.size());
}

void uniforms_copy(Uniforms &destination, const Uniforms &source) {
    for (const auto &[name, field] : destination.fields) {
        auto it = source.fields.find(name);
        if (it != source.fields.end() && it->second.kind == field.kind)
            std::memcpy(destination.data.data() + field.offset,
                        source.data.data() + it->second.offset, field.size);
    }
}
