#pragma once
#include "devices.hpp"
struct Timestamps {
    VkQueryPool pool{};
    bool submitted{};
};
void timestamps_create(const Device &, Timestamps &);
void timestamps_destroy(const Device &, Timestamps &);
bool timestamps_read(const Device &, const Timestamps &, double &ms);
