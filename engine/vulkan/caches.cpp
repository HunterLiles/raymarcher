#include "caches.hpp"
#include "images.hpp"
#include <algorithm>
#include <cmath>

static constexpr VkDeviceSize entry_bytes = gpu_vec4.size;

static VkDeviceSize cache_limit(const Device &d, const Caches &c) {
    return std::min(c.limit, VkDeviceSize(d.properties.limits.maxStorageBufferRange));
}

static void cache_barrier(VkCommandBuffer cmd, VkAccessFlags src, VkAccessFlags dst) {
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = src;
    barrier.dstAccessMask = dst;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0, nullptr, 0,
                         nullptr);
}

void cache_add(const Device &d, Caches &c, VkPipelineLayout layout, uint32_t id,
               const std::filesystem::path &path, uint32_t count, float fps) {
    if (!count || fps < 0.0f || !std::isfinite(fps))
        throw std::runtime_error("Cache needs entries and a finite, nonnegative rate");

    if (id < c.jobs.size() && c.jobs[id].pipeline)
        throw std::runtime_error("Cache ID already registered");

    uint64_t entries = std::max<uint64_t>(c.jobs.size(), uint64_t(id) + 1) + count;

    for (const auto &job : c.jobs)
        entries += job.count;

    if (entries > cache_limit(d, c) / entry_bytes)
        throw std::runtime_error("Cache memory budget exceeded");

    auto program = c.programs.find(path);

    if (program == c.programs.end()) {
        VkPipeline pipeline = compute_pipeline_create(d, layout, path);

        try {
            program = c.programs.emplace(path, pipeline).first;
        } catch (...) {
            vkDestroyPipeline(d.handle, pipeline, nullptr);
            throw;
        }
    }

    c.jobs.resize(std::max(c.jobs.size(), std::size_t(id) + 1));
    c.jobs[id] = {program->second, 0, count, fps, -1};
    c.used = entries * entry_bytes;
    c.dirty = true;
}

bool caches_prepare(const Device &d, Caches &c) {
    if (!c.dirty)
        return false;

    uint64_t entries = c.jobs.size();

    for (auto &job : c.jobs) {
        job.offset = uint32_t(entries);
        entries += job.count;
        job.tick = -1;
    }

    c.used = std::max<VkDeviceSize>(entries * entry_bytes, entry_bytes);
    VkDeviceSize limit = cache_limit(d, c);

    if (c.used > limit)
        throw std::runtime_error("Cache memory budget exceeded");

    bool resized = c.used > c.capacity || c.allocated > limit;

    if (resized) {
        VkDeviceSize capacity = std::max(c.capacity, entry_bytes);

        while (capacity < c.used)
            capacity = std::min(capacity * 2, limit);

        capacity = std::min(capacity, limit);

        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        info.size = capacity;
        info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer buffer{};
        VK_CHECK(vkCreateBuffer(d.handle, &info, nullptr, &buffer));

        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(d.handle, buffer, &requirements);

        if (requirements.size > limit) {
            vkDestroyBuffer(d.handle, buffer, nullptr);
            throw std::runtime_error("Cache allocation padding exceeds budget");
        }

        try {
            VK_CHECK(vkDeviceWaitIdle(d.handle));
        } catch (...) {
            vkDestroyBuffer(d.handle, buffer, nullptr);
            throw;
        }

        if (c.buffer)
            vkDestroyBuffer(d.handle, c.buffer, nullptr);

        if (c.memory)
            vkFreeMemory(d.handle, c.memory, nullptr);

        c.buffer = VK_NULL_HANDLE;
        c.memory = VK_NULL_HANDLE;
        c.capacity = c.allocated = 0;

        VkDeviceMemory memory{};

        try {
            VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            allocation.allocationSize = requirements.size;
            allocation.memoryTypeIndex = memory_type(d.physical, requirements.memoryTypeBits,
                                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            VK_CHECK(vkAllocateMemory(d.handle, &allocation, nullptr, &memory));
            VK_CHECK(vkBindBufferMemory(d.handle, buffer, memory, 0));
        } catch (...) {
            vkDestroyBuffer(d.handle, buffer, nullptr);

            if (memory)
                vkFreeMemory(d.handle, memory, nullptr);

            throw;
        }

        c.buffer = buffer;
        c.memory = memory;
        c.capacity = capacity;
        c.allocated = requirements.size;
    }

    c.dirty = false;
    return resized;
}

void caches_record(const Device &d, Caches &c, VkCommandBuffer cmd, VkPipelineLayout layout,
                   VkDescriptorSet descriptor, float time) {
    bool updating = false;

    for (uint32_t id = 0; id < c.jobs.size(); ++id) {
        auto &job = c.jobs[id];

        if (!job.pipeline)
            continue;

        int64_t tick = job.fps > 0.0f ? int64_t(std::floor(double(time) * job.fps)) : 0;

        if (tick == job.tick)
            continue;

        if (!updating) {
            cache_barrier(cmd, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                          VK_ACCESS_SHADER_WRITE_BIT);

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, &descriptor,
                                    0, nullptr);
            updating = true;
        }

        CacheDispatch data{.time = job.fps > 0.0f ? float(tick) / job.fps : 0.0f,
                           .offset = job.offset,
                           .count = job.count,
                           .id = id};

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, job.pipeline);
        vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(data), &data);

        uint32_t groups = (job.count - 1) / 64 + 1;
        uint32_t x = std::min(groups, d.properties.limits.maxComputeWorkGroupCount[0]);
        uint32_t y = (groups - 1) / x + 1;

        if (y > d.properties.limits.maxComputeWorkGroupCount[1])
            throw std::runtime_error("Cache dispatch exceeds device limits");

        vkCmdDispatch(cmd, x, y, 1);
        job.tick = tick;
    }

    if (updating)
        cache_barrier(cmd, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
}

void caches_destroy(const Device &d, Caches &c) {
    for (const auto &[path, pipeline] : c.programs)
        vkDestroyPipeline(d.handle, pipeline, nullptr);

    if (c.buffer)
        vkDestroyBuffer(d.handle, c.buffer, nullptr);

    if (c.memory)
        vkFreeMemory(d.handle, c.memory, nullptr);

    c = {};
}
