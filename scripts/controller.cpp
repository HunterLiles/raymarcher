#include "controller.hpp"
#include <GLFW/glfw3.h>

void Controller::camera_move(Camera &camera, float dt, GLFWwindow *window) {
    vec3 move{};

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        move += vec3(0.0, 0.0, 1.0);

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        move -= vec3(0.0, 0.0, 1.0);

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        move += vec3(1.0, 0.0, 0.0);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        move -= vec3(1.0, 0.0, 0.0);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        move += vec3(0.0, 1.0, 0.0);

    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        move -= vec3(0.0, 1.0, 0.0);

    move = normalize(move);

    float dist = speed * dt;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        dist *= sprint;

    // rotate() keeps the movement relative to the camera
    camera.pos += rotate(camera.orientation, move) * dist;
}

void Controller::camera_rotate(Camera &camera, GLFWwindow *window) {
    double x{}, y{};
    glfwGetCursorPos(window, &x, &y);

    if (!mouse_ready) {
        mouse_x = x;
        mouse_y = y;
        mouse_ready = true;
        return;
    }

    float yaw = float(x - mouse_x) * sens;
    float pitch = float(y - mouse_y) * sens;
    mouse_x = x;
    mouse_y = y;

    camera.orientation = normalize(axis_angle({0.0f, 1.0f, 0.0f}, yaw) * camera.orientation *
                                   axis_angle({1.0f, 0.0f, 0.0f}, pitch));
}
