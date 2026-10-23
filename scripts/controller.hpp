#pragma once

#include "camera.hpp"

struct GLFWwindow;

class Controller {
  private:
    double mouse_x{}, mouse_y{};
    bool mouse_ready{};

  public:
    float speed{10.0f}, sprint{1.5f}, sens{0.0025f};

    void camera_move(Camera &, float dt, GLFWwindow *);
    void camera_rotate(Camera &, GLFWwindow *);
};
