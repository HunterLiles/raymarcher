#include "timestamps.hpp"
void timestamps_create(const Device &d, Timestamps &t) {
    if (!d.queues.timestamp_bits)
        return;
    VkQueryPoolCreateInfo ci{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    ci.queryType = VK_QUERY_TYPE_TIMESTAMP;
    ci.queryCount = 2;
    VK_CHECK(vkCreateQueryPool(d.handle, &ci, nullptr, &t.pool));
}
void timestamps_destroy(const Device &d, Timestamps &t) {
    if (t.pool)
        vkDestroyQueryPool(d.handle, t.pool, nullptr);
    t = {};
}
bool timestamps_read(const Device &d, const Timestamps &t, double &ms) {
    if (!t.pool || !t.submitted)
        return false;
    uint64_t values[2]{};
    auto result = vkGetQueryPoolResults(d.handle, t.pool, 0, 2, sizeof(values), values,
                                        sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
    if (result == VK_NOT_READY)
        return false;
    VK_CHECK(result);
    uint32_t bits = d.queues.timestamp_bits;
    uint64_t mask = bits == 64 ? UINT64_MAX : ((uint64_t(1) << bits) - 1);
    ms = double((values[1] - values[0]) & mask) * double(d.properties.limits.timestampPeriod) / 1e6;
    return true;
}
