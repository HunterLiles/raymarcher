#pragma once

#include "../engine.hpp"
#include "pipelines.hpp"
#include <map>
#include <vector>

struct CacheJob {
    VkPipeline pipeline{};
    uint32_t offset{}, count{};
    float fps{};
    int64_t tick{-1};
};

struct Caches {
    VkBuffer buffer{};
    VkDeviceMemory memory{};
    VkDeviceSize used{}, capacity{}, allocated{};
    VkDeviceSize limit{engine::EngineConfig{}.cache_budget};

    std::vector<CacheJob> jobs;
    std::map<std::filesystem::path, VkPipeline> programs;
    bool dirty{true};
};

void cache_add(const Device &, Caches &, VkPipelineLayout, uint32_t id,
               const std::filesystem::path &, uint32_t count, float fps);

bool caches_prepare(const Device &, Caches &);

void caches_record(const Device &, Caches &, VkCommandBuffer, VkPipelineLayout, VkDescriptorSet,
                   float time);

void caches_destroy(const Device &, Caches &);
