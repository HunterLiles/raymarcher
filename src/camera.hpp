#pragma once

#include "engine_math.hpp"

struct Camera {
    vec3 pos{0.0f, 2.0f, -5.0f};
    quaternion orientation{};
};
