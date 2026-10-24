#pragma once

#include <engine/engine_math.hpp>

struct Player {
    engine::vec2 health{100.0f, 100.0f};
    engine::vec2 mana{100.0f, 100.0f};
    engine::vec2 stamina{100.0f, 100.0f};
};

struct Camera {
    engine::vec3 pos{0.0f, 2.0f, -5.0f};
    engine::quaternion orientation{};
};
