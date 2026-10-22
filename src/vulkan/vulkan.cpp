#include "vulkan.hpp"
#include "commands.hpp"
#include "images.hpp"
#include "instance.hpp"
#include "pipelines.hpp"
#include "swapchain.hpp"
#include "synchronization.hpp"
#include "timestamps.hpp"
#include <array>
#include <chrono>

using Clock = std::chrono::steady_clock;

static double elapsed(Clock::time_point t) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
}

struct Frame {
    Commands commands;
    FrameSync sync;
    Timestamps timestamps;

    RenderImage output;
    VkDescriptorSet descriptor{};
};

struct Vulkan {
    Instance instance;
    VkSurfaceKHR surface{};
    Device device;
    Swapchain swapchain;
    Pipeline pipeline;
    VkDescriptorPool descriptor_pool{};

    std::array<Frame, 2> frames{};
    uint32_t slot{};
    uint32_t requested_width{}, requested_height{};
    bool recreate{};

    std::filesystem::path shaders;
    FrameStats stats;
};

static void descriptors_create(Vulkan &v) {
    auto device = v.device.handle;
    uint32_t count = uint32_t(v.frames.size());

    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, count};

    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.maxSets = count;
    pool.poolSizeCount = 1;
    pool.pPoolSizes = &size;

    VK_CHECK(vkCreateDescriptorPool(device, &pool, nullptr, &v.descriptor_pool));

    for (auto &f : v.frames) {
        VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocation.descriptorPool = v.descriptor_pool;
        allocation.descriptorSetCount = 1;
        allocation.pSetLayouts = &v.pipeline.descriptor_layout;

        VK_CHECK(vkAllocateDescriptorSets(device, &allocation, &f.descriptor));

        VkDescriptorImageInfo image{};
        image.imageView = f.output.view;
        image.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = f.descriptor;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        write.pImageInfo = &image;

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
}

static void frame_create(const Device &d, Frame &f) {
    commands_create(d, f.commands);
    sync_create(d, f.sync);
    timestamps_create(d, f.timestamps);
    image_create(d, f.output);
}

// The caller waits for the device before destroying frame resources.
static void frame_destroy(const Device &d, Frame &f) {
    image_destroy(d, f.output);
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

    v.requested_width = w;
    v.requested_height = h;
    v.recreate = false;
}

void vulkan_initialize(Vulkan &v, VkSurfaceKHR surface, uint32_t w, uint32_t h,
                       const std::filesystem::path &shaders) {
    v.surface = surface;
    v.shaders = shaders;

    device_create(v.device, v.instance.handle, surface);
    pipeline_create(v.device, v.pipeline, v.shaders);

    for (auto &f : v.frames)
        frame_create(v.device, f);

    descriptors_create(v);
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

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, v.pipeline.handle);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, v.pipeline.layout, 0, 1,
                            &f.descriptor, 0, nullptr);

    FrameData data{320.0f, 180.0f, time};
    vkCmdPushConstants(cmd, v.pipeline.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(data), &data);
    vkCmdDispatch(cmd, 40, 23, 1);

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
    region.srcOffsets[1] = {320, 180, 1};
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

    if (v.recreate || w != v.requested_width || h != v.requested_height)
        rebuild(v, w, h);

    auto &f = v.frames[v.slot];
    uint32_t image = 0;
    if (!frame_acquire(v, f, image))
        return false;

    auto start = Clock::now();
    frame_record(v, f, image, time);
    frame_submit(v, f, image);
    v.stats.cpu_work_ms = elapsed(start);
    frame_present(v, image);

    ++v.stats.submitted_frames;
    v.slot = (v.slot + 1) % uint32_t(v.frames.size());

    return true;
}

FrameStats vulkan_stats(const Vulkan &v) {
    return v.stats;
}
