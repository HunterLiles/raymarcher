#pragma once

#include "player.hpp"
#include <engine/engine.hpp>

struct GLFWwindow;

class Controller {
  private:
    double mouse_x{}, mouse_y{};
    bool mouse_ready{}, jump_held{};
    float vert_speed{}, crouch{0.65}, crouch_amount{};

  public:
    float speed{10.0f}, sprint{1.5f}, sens{0.0025f};
    float gravity{9.81f}, jump{5.0f};
    float height{1.7f};

    Player player{};

    void camera_move(Camera &, float dt, GLFWwindow *);
    void camera_rotate(Camera &, GLFWwindow *);
};
