#include "vulkan.hpp"
#include "../hot_reload.hpp"
#include "caches.hpp"
#include "commands.hpp"
#include "images.hpp"
#include "instance.hpp"
#include "pipelines.hpp"
#include "swapchain.hpp"
#include "synchronization.hpp"
#include "timestamps.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <utility>

using engine::FrameStats;

using Clock = std::chrono::steady_clock;

static double elapsed(Clock::time_point t) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
}

struct Frame {
    Commands commands;
    FrameSync sync;
    Timestamps timestamps;

    RenderImage output, shadow;
    VkDescriptorSet descriptor{};
};

struct Vulkan {
    Instance instance;
    VkSurfaceKHR surface{};
    Device device;
    Swapchain swapchain;
    Pipeline pipeline;
    Caches caches;
    VkDescriptorPool descriptor_pool{};

    std::array<Frame, 2> frames{};
    uint32_t slot{};
    uint32_t requested_width{}, requested_height{};
    bool recreate{};

    uint32_t pixel_size{}, shadow_pixel, size{};
    FrameStats stats;
};

static void cache_descriptors_update(Vulkan &v) {
    VkDescriptorBufferInfo buffer{v.caches.buffer, 0, v.caches.capacity};

    for (auto &f : v.frames) {
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = f.descriptor;
        write.dstBinding = 1;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo = &buffer;

        vkUpdateDescriptorSets(v.device.handle, 1, &write, 0, nullptr);
    }
}

static void descriptors_build(Vulkan &v, const Pipeline &pipeline, VkBuffer buffer,
                              VkDeviceSize capacity, VkDescriptorPool &handle,
                              std::array<VkDescriptorSet, 2> &sets) {
    auto device = v.device.handle;
    uint32_t count = uint32_t(v.frames.size());

    VkDescriptorPoolSize sizes[] = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, count * 2},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, count},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count},
    };

    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.maxSets = count;
    pool.poolSizeCount = 3;
    pool.pPoolSizes = sizes;

    VK_CHECK(vkCreateDescriptorPool(device, &pool, nullptr, &handle));

    for (uint32_t i = 0; i < count; ++i) {
        VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocation.descriptorPool = handle;
        allocation.descriptorSetCount = 1;
        allocation.pSetLayouts = &pipeline.descriptor_layout;

        VK_CHECK(vkAllocateDescriptorSets(device, &allocation, &sets[i]));

        VkDescriptorImageInfo image{};
        image.imageView = v.frames[i].output.view;
        image.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkDescriptorImageInfo shadow{};
        shadow.imageView = v.frames[i].shadow.view;
        shadow.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkDescriptorBufferInfo storage{buffer, 0, capacity};
        VkDescriptorBufferInfo uniforms{pipeline.uniforms.buffers[i].handle, 0,
                                        pipeline.uniforms.data.size()};
        VkWriteDescriptorSet writes[4]{};

        for (auto &write : writes) {
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = sets[i];
            write.descriptorCount = 1;
        }

        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[0].pImageInfo = &image;

        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].pBufferInfo = &storage;

        writes[2].dstBinding = 2;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[2].pBufferInfo = &uniforms;

        writes[3].dstBinding = 3;
        writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[3].pImageInfo = &shadow;

        vkUpdateDescriptorSets(device, 4, writes, 0, nullptr);
    }
}

static void descriptors_create(Vulkan &v) {
    std::array<VkDescriptorSet, 2> sets{};

    descriptors_build(v, v.pipeline, v.caches.buffer, v.caches.capacity, v.descriptor_pool, sets);

    for (uint32_t i = 0; i < sets.size(); ++i)
        v.frames[i].descriptor = sets[i];
}
static void frame_create(const Device &d, Frame &f) {
    commands_create(d, f.commands);
    sync_create(d, f.sync);
    timestamps_create(d, f.timestamps);
}

// The caller waits for the device before destroying frame.
static void frame_destroy(const Device &d, Frame &f) {
    image_destroy(d, f.output);
    image_destroy(d, f.shadow);
    timestamps_destroy(d, f.timestamps);
    sync_destroy(d, f.sync);
    commands_destroy(d, f.commands);
}

Vulkan *vulkan_create(std::span<const char *const> extensions, bool validation) {
    auto *v = new Vulkan{};

    try {
        instance_create(v->instance, extensions, validation);
    } catch (...) {
        vulkan_destroy(v);
        throw;
    }

    return v;
}

VkInstance vulkan_instance(const Vulkan &v) {
    return v.instance.handle;
}

static void rebuild(Vulkan &v, uint32_t w, uint32_t h) {
    auto &d = v.device;
    VK_CHECK(vkDeviceWaitIdle(d.handle));

    swapchain_destroy(d, v.swapchain);
    swapchain_create(d, v.swapchain, v.surface, w, h);

    if (v.descriptor_pool) {
        vkDestroyDescriptorPool(d.handle, v.descriptor_pool, nullptr);
        v.descriptor_pool = {};
    }

    VkExtent2D extent =
        render_extent(v.swapchain.extent.width, v.swapchain.extent.height, v.pixel_size);

    for (auto &f : v.frames) {
        image_destroy(d, f.output);
        image_destroy(d, f.shadow);

        image_create(d, f.output, extent);
        image_create(d, f.shadow, render_extent(extent.width, extent.height, v.shadow_pixel));
    }

    descriptors_create(v);

    v.requested_width = w;
    v.requested_height = h;
    v.recreate = false;
}

void vulkan_initialize(Vulkan &v, VkSurfaceKHR surface, uint32_t w, uint32_t h,
                       const std::filesystem::path &shader, uint32_t pixel_size,
                       uint32_t shadow_pixel_size, const ShaderBuild *build) {
    v.surface = surface;
    v.pixel_size = pixel_size;
    v.shadow_pixel = shadow_pixel_size;

    device_create(v.device, v.instance.handle, surface);
    pipeline_create(v.device, v.pipeline, shader);

    if (build)
        for (const auto &cache : build->caches)
            cache_add(v.device, v.caches, v.pipeline.layout, cache.id, build->cache, cache.count,
                      cache.fps);

    caches_prepare(v.device, v.caches);

    for (auto &f : v.frames)
        frame_create(v.device, f);

    rebuild(v, w, h);
    v.stats.device_name = v.device.properties.deviceName;
}

void vulkan_destroy(Vulkan *v) {
    if (!v)
        return;

    auto &d = v->device;
    if (d.handle)
        vkDeviceWaitIdle(d.handle);

    if (v->descriptor_pool)
        vkDestroyDescriptorPool(d.handle, v->descriptor_pool, nullptr);

    caches_destroy(d, v->caches);
    pipeline_destroy(d, v->pipeline);

    for (auto &f : v->frames)
        frame_destroy(d, f);

    swapchain_destroy(d, v->swapchain);
    device_destroy(d);

    if (v->surface)
        vkDestroySurfaceKHR(v->instance.handle, v->surface, nullptr);

    instance_destroy(v->instance);
    delete v;
}

static bool frame_acquire(Vulkan &v, Frame &f, uint32_t &image) {
    VK_CHECK(vkWaitForFences(v.device.handle, 1, &f.sync.complete, VK_TRUE, UINT64_MAX));
    v.stats.gpu_available = timestamps_read(v.device, f.timestamps, v.stats.gpu_ms);

    auto result = vkAcquireNextImageKHR(v.device.handle, v.swapchain.handle, UINT64_MAX,
                                        f.sync.acquired, VK_NULL_HANDLE, &image);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        v.recreate = true;
        return false;
    }

    if (result == VK_SUBOPTIMAL_KHR)
        v.recreate = true;
    else
        VK_CHECK(result);

    VK_CHECK(vkResetCommandPool(v.device.handle, f.commands.pool, 0));
    return true;
}

static void record_compute(Vulkan &v, Frame &f, float time) {
    auto cmd = f.commands.buffer;
    auto &output = f.output;

    image_barrier(
        cmd, output.handle, VK_IMAGE_ASPECT_COLOR_BIT,
        output.initialized ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL, output.initialized ? VK_ACCESS_TRANSFER_READ_BIT : 0,
        VK_ACCESS_SHADER_WRITE_BIT,
        output.initialized ? VK_PIPELINE_STAGE_TRANSFER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    auto &shadow = f.shadow;
    image_barrier(cmd, shadow.handle, VK_IMAGE_ASPECT_COLOR_BIT,
                  shadow.initialized ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_GENERAL, shadow.initialized ? VK_ACCESS_SHADER_READ_BIT : 0,
                  VK_ACCESS_SHADER_WRITE_BIT,
                  shadow.initialized ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
                                     : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    caches_record(v.device, v.caches, cmd, v.pipeline.layout, f.descriptor, time);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, v.pipeline.handle);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, v.pipeline.layout, 0, 1,
                            &f.descriptor, 0, nullptr);

    FrameData data{};
    data.resolution = {float(v.swapchain.extent.width), float(v.swapchain.extent.height)};
    data.time = time;

    data.pass = 1;
    vkCmdPushConstants(cmd, v.pipeline.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(data), &data);
    vkCmdDispatch(cmd, (shadow.extent.width + 7) / 8, (shadow.extent.height + 7) / 8, 1);

    image_barrier(cmd, shadow.handle, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL,
                  VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    data.pass = 0;

    vkCmdPushConstants(cmd, v.pipeline.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(data), &data);
    vkCmdDispatch(cmd, (output.extent.width + 7) / 8, (output.extent.height + 7) / 8, 1);

    image_barrier(cmd, output.handle, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL,
                  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_SHADER_WRITE_BIT,
                  VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                  VK_PIPELINE_STAGE_TRANSFER_BIT);
}

static void record_blit(Vulkan &v, Frame &f, uint32_t image) {
    auto cmd = f.commands.buffer;
    auto &s = v.swapchain;

    // Acquisition orders access; the full-image blit replaces the previous contents.
    image_barrier(cmd, s.images[image], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkImageBlit region{};
    region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.srcOffsets[0] = {0, 0, 0};
    region.srcOffsets[1] = {int32_t(f.output.extent.width), int32_t(f.output.extent.height), 1};
    region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.dstOffsets[0] = {0, 0, 0};
    region.dstOffsets[1] = {int32_t(s.extent.width), int32_t(s.extent.height), 1};

    vkCmdBlitImage(cmd, f.output.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, s.images[image],
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_NEAREST);

    image_barrier(cmd, s.images[image], VK_IMAGE_ASPECT_COLOR_BIT,
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                  VK_ACCESS_TRANSFER_WRITE_BIT, 0, VK_PIPELINE_STAGE_TRANSFER_BIT,
                  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
}

static void frame_record(Vulkan &v, Frame &f, uint32_t image, float time) {
    auto cmd = f.commands.buffer;
    commands_begin(cmd);
    if (f.timestamps.pool) {
        vkCmdResetQueryPool(cmd, f.timestamps.pool, 0, 2);
        vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, f.timestamps.pool, 0);
    }

    record_compute(v, f, time);
    record_blit(v, f, image);

    if (f.timestamps.pool)
        vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, f.timestamps.pool, 1);
    VK_CHECK(vkEndCommandBuffer(cmd));
}

static void frame_submit(Vulkan &v, Frame &f, uint32_t image) {
    auto &s = v.swapchain;
    // Start GPU timestamps only after acquisition is complete.
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &f.sync.acquired;
    submit.pWaitDstStageMask = &wait_stage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &f.commands.buffer;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &s.present_ready[image];

    // Reset only when a submission will actually signal the fence.
    VK_CHECK(vkResetFences(v.device.handle, 1, &f.sync.complete));
    VK_CHECK(vkQueueSubmit(v.device.queues.graphics, 1, &submit, f.sync.complete));

    f.output.initialized = true;
    f.shadow.initialized = true;
    f.timestamps.submitted = true;
}

static void frame_present(Vulkan &v, uint32_t image) {
    auto &s = v.swapchain;
    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &s.present_ready[image];
    present.swapchainCount = 1;
    present.pSwapchains = &s.handle;
    present.pImageIndices = &image;

    auto result = vkQueuePresentKHR(v.device.queues.present, &present);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        v.recreate = true;
    else
        VK_CHECK(result);
}

bool vulkan_draw(Vulkan &v, uint32_t w, uint32_t h, float time) {
    if (!w || !h)
        return false;

    if (caches_prepare(v.device, v.caches))
        cache_descriptors_update(v);

    if (v.recreate || w != v.requested_width || h != v.requested_height)
        rebuild(v, w, h);

    auto &f = v.frames[v.slot];
    uint32_t image = 0;
    if (!frame_acquire(v, f, image))
        return false;

    uniforms_upload(v.pipeline.uniforms, v.slot);

    auto start = Clock::now();
    frame_record(v, f, image, time);
    frame_submit(v, f, image);
    v.stats.cpu_work_ms = elapsed(start);
    frame_present(v, image);

    ++v.stats.submitted_frames;
    v.slot = (v.slot + 1) % uint32_t(v.frames.size());

    return true;
}

void vulkan_set_uniform(Vulkan &v, std::string_view name, const void *value, UniformKind kind) {
    uniforms_set(v.pipeline.uniforms, name, value, kind);
}

FrameStats vulkan_stats(const Vulkan &v) {
    return v.stats;
}

void vulkan_cache_create(Vulkan &v, uint32_t id, const std::filesystem::path &shader,
                         uint32_t count, float fps) {
    cache_add(v.device, v.caches, v.pipeline.layout, id, shader.lexically_normal(), count, fps);
}

void vulkan_cache_budget(Vulkan &v, VkDeviceSize bytes) {
    v.caches.limit = bytes;
    v.caches.dirty = true;
}

engine::CacheStats vulkan_cache_stats(const Vulkan &v) {
    auto limit =
        std::min(v.caches.limit, VkDeviceSize(v.device.properties.limits.maxStorageBufferRange));

    return {v.caches.used, v.caches.allocated, limit};
}

void vulkan_reload(Vulkan &v, const ShaderBuild &build) {
    auto &d = v.device;

    Pipeline next_pipeline;
    Caches next_caches;
    next_caches.limit = v.caches.limit;

    VkDescriptorPool next_pool{};
    std::array<VkDescriptorSet, 2> next_sets{};

    try {
        pipeline_create(d, next_pipeline, build.render);
        uniforms_copy(next_pipeline.uniforms, v.pipeline.uniforms);

        for (const auto &cache : build.caches)
            cache_add(d, next_caches, next_pipeline.layout, cache.id, build.cache, cache.count,
                      cache.fps);

        bool reuse = next_caches.used <= v.caches.capacity;

        if (reuse) {
            // Prepare offsets without taking ownership of the live buffer.
            next_caches.capacity = v.caches.capacity;
            next_caches.allocated = v.caches.allocated;
        } else {
            // Both allocations must fit while staging a larger replacement.
            next_caches.limit = v.caches.limit - v.caches.allocated;
        }

        caches_prepare(d, next_caches);
        next_caches.limit = v.caches.limit;

        descriptors_build(v, next_pipeline, reuse ? v.caches.buffer : next_caches.buffer,
                          next_caches.capacity, next_pool, next_sets);

        VK_CHECK(vkDeviceWaitIdle(d.handle));

        if (reuse) {
            next_caches.buffer = std::exchange(v.caches.buffer, VK_NULL_HANDLE);
            next_caches.memory = std::exchange(v.caches.memory, VK_NULL_HANDLE);
        }

        std::swap(v.pipeline, next_pipeline);
        std::swap(v.caches, next_caches);
        std::swap(v.descriptor_pool, next_pool);

        for (uint32_t i = 0; i < next_sets.size(); ++i)
            v.frames[i].descriptor = next_sets[i];
    } catch (...) {
        if (next_pool)
            vkDestroyDescriptorPool(d.handle, next_pool, nullptr);

        caches_destroy(d, next_caches);
        pipeline_destroy(d, next_pipeline);
        throw;
    }

    vkDestroyDescriptorPool(d.handle, next_pool, nullptr);
    caches_destroy(d, next_caches);
    pipeline_destroy(d, next_pipeline);
}
