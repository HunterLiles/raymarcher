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
};
struct Vulkan {
    Instance instance;
    VkSurfaceKHR surface{};
    Device device;
    Swapchain swapchain;
    Pipeline pipeline;
    std::array<Frame, 2> frames{};
    uint32_t slot{}, image{}, generation{};
    uint32_t requested_width{}, requested_height{};
    bool recreate{}, frame_open{};
    std::filesystem::path shaders;
    FrameStats stats;
};
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
    pipeline_destroy(d, v.pipeline);
    swapchain_destroy(d, v.swapchain);
    swapchain_create(d, v.swapchain, v.surface, w, h);
    pipeline_create(d, v.pipeline, v.swapchain.format, v.shaders);
    v.requested_width = w;
    v.requested_height = h;
    v.recreate = false;
    ++v.generation;
    v.stats.width = v.swapchain.extent.width;
    v.stats.height = v.swapchain.extent.height;
    v.stats.swapchain_images = uint32_t(v.swapchain.images.size());
}
void vulkan_initialize(Vulkan &v, VkSurfaceKHR surface, uint32_t w, uint32_t h,
                       const std::filesystem::path &shaders) {
    v.surface = surface;
    v.shaders = shaders;
    device_create(v.device, v.instance.handle, surface);
    for (auto &f : v.frames) {
        commands_create(v.device, f.commands);
        sync_create(v.device, f.sync);
        timestamps_create(v.device, f.timestamps);
    }
    rebuild(v, w, h);
    v.stats.device_name = v.device.properties.deviceName;
    v.stats.validation = v.instance.validation;
}
void vulkan_wait_idle(Vulkan &v) {
    if (v.device.handle)
        VK_CHECK(vkDeviceWaitIdle(v.device.handle));
}
void vulkan_destroy(Vulkan *v) {
    if (!v)
        return;
    auto &d = v->device;
    if (d.handle)
        vkDeviceWaitIdle(d.handle);
    pipeline_destroy(d, v->pipeline);
    for (auto &f : v->frames) {
        timestamps_destroy(d, f.timestamps);
        sync_destroy(d, f.sync);
        commands_destroy(d, f.commands);
    }
    swapchain_destroy(d, v->swapchain);
    device_destroy(d);
    if (v->surface)
        vkDestroySurfaceKHR(v->instance.handle, v->surface, nullptr);
    instance_destroy(v->instance);
    delete v;
}
bool vulkan_begin(Vulkan &v, uint32_t w, uint32_t h) {
    if (!w || !h)
        return false;
    if (v.frame_open)
        throw std::runtime_error("Frame already open");
    auto start = Clock::now();
    if (v.recreate || w != v.requested_width || h != v.requested_height)
        rebuild(v, w, h);
    auto &f = v.frames[v.slot];
    VK_CHECK(vkWaitForFences(v.device.handle, 1, &f.sync.complete, VK_TRUE, UINT64_MAX));
    v.stats.gpu_available = timestamps_read(v.device, f.timestamps, v.stats.gpu_ms);
    auto result = vkAcquireNextImageKHR(v.device.handle, v.swapchain.handle, UINT64_MAX,
                                        f.sync.acquired, VK_NULL_HANDLE, &v.image);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        v.recreate = true;
        return false;
    }
    if (result == VK_SUBOPTIMAL_KHR)
        v.recreate = true;
    else
        VK_CHECK(result);
    VK_CHECK(vkResetCommandPool(v.device.handle, f.commands.pool, 0));
    v.stats.wait_ms = elapsed(start);
    v.frame_open = true;
    return true;
}
void vulkan_draw(Vulkan &v, const RenderData &data, OverlayRecorder overlay, void *user,
                 double frame_ms, double app_cpu_ms) {
    if (!v.frame_open)
        throw std::runtime_error("Invalid render data/frame state");
    auto start = Clock::now();
    auto &f = v.frames[v.slot];
    auto cmd = f.commands.buffer;
    auto &s = v.swapchain;
    commands_begin(cmd);
    if (f.timestamps.pool) {
        vkCmdResetQueryPool(cmd, f.timestamps.pool, 0, 2);
        vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, f.timestamps.pool, 0);
    }
    // Prior image contents are discarded; acquisition and the semaphore provide ownership/order.
    image_barrier(cmd, s.images[v.image], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    VkRenderingAttachmentInfo color{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    color.imageView = s.views[v.image];
    color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.clearValue.color = {{.018f, .024f, .04f, 1}};
    VkRenderingInfo rendering{VK_STRUCTURE_TYPE_RENDERING_INFO};
    rendering.renderArea = {{0, 0}, s.extent};
    rendering.layerCount = 1;
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachments = &color;
    vkCmdBeginRendering(cmd, &rendering);
    VkViewport viewport{0, 0, float(s.extent.width), float(s.extent.height), 0, 1};
    VkRect2D scissor{{0, 0}, s.extent};
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, v.pipeline.handle);
    vkCmdPushConstants(cmd, v.pipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(data),
                       &data);
    vkCmdDraw(cmd, 3, 1, 0, 0);
    vkCmdEndRendering(cmd);
    if (overlay) {
        // Explicit attachment dependency between the scene and overlay rendering scopes.
        image_barrier(cmd, s.images[v.image], VK_IMAGE_ASPECT_COLOR_BIT,
                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        color.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        rendering.pDepthAttachment = nullptr;
        vkCmdBeginRendering(cmd, &rendering);
        overlay(cmd, user);
        vkCmdEndRendering(cmd);
    }
    image_barrier(
        cmd, s.images[v.image], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    if (f.timestamps.pool)
        vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, f.timestamps.pool, 1);
    VK_CHECK(vkEndCommandBuffer(cmd));
    // Start GPU timestamps only after acquisition is complete.
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &f.sync.acquired;
    submit.pWaitDstStageMask = &wait_stage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &s.present_ready[v.image];
    // Reset only when a submission will actually signal the fence.
    VK_CHECK(vkResetFences(v.device.handle, 1, &f.sync.complete));
    VK_CHECK(vkQueueSubmit(v.device.queues.graphics, 1, &submit, f.sync.complete));
    f.timestamps.submitted = true;
    v.stats.cpu_work_ms = app_cpu_ms + elapsed(start);
    auto present_start = Clock::now();
    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &s.present_ready[v.image];
    present.swapchainCount = 1;
    present.pSwapchains = &s.handle;
    present.pImageIndices = &v.image;
    auto result = vkQueuePresentKHR(v.device.queues.present, &present);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        v.recreate = true;
    else
        VK_CHECK(result);
    v.stats.wait_ms += elapsed(present_start);
    v.stats.frame_ms = frame_ms;
    v.stats.draws = 1;
    v.stats.triangles = 1;
    ++v.stats.submitted_frames;
    v.slot = (v.slot + 1) % uint32_t(v.frames.size());
    v.frame_open = false;
}
GuiDeviceInfo vulkan_gui_info(const Vulkan &v) {
    return {v.instance.handle,
            v.device.physical,
            v.device.handle,
            v.device.queues.graphics,
            v.device.queues.graphics_family,
            v.swapchain.format,
            v.swapchain.min_images,
            uint32_t(v.swapchain.images.size()),
            v.generation};
}
FrameStats vulkan_stats(const Vulkan &v) {
    return v.stats;
}
float vulkan_aspect(const Vulkan &v) {
    return float(v.swapchain.extent.width) / float(v.swapchain.extent.height);
}
