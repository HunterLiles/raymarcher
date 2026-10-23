#include "controller.hpp"
#include <GLFW/glfw3.h>
#include <cmath>

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

    // if looking up, dont move up
    vec3 forward = rotate(camera.orientation, {0.0f, 0.0f, 1.0f});
    forward.y = 0.0f;
    forward = normalize(forward);

    vec3 right = normalize(cross({0.0f, 1.0f, 0.0f}, forward));
    move = normalize(right * move.x + forward * move.z);

    float dist = speed * dt;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        dist *= sprint;

    // rotate() keeps the movement relative to the camera
    camera.pos += move * dist;

    // crouch
    float prev_floor = std::lerp(height, height * crouch, crouch_amount);

    bool isCrouching = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
    crouch_amount = std::lerp(crouch_amount, float(isCrouching), 1.0f - std::exp(-12.0f * dt));

    // gravity and jumping
    float floor = std::lerp(height, height * crouch, crouch_amount);
    camera.pos.y += floor - prev_floor;
    bool grounded = camera.pos.y <= floor && vert_speed <= 0.0f;

    if (grounded) {
        camera.pos.y = floor;
        vert_speed = 0.0f;
    }

    bool isJump = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

    if (isJump && !jump_held && grounded)
        vert_speed = jump;

    jump_held = isJump;

    camera.pos.y += vert_speed * dt - 0.5f * gravity * dt * dt;
    vert_speed -= gravity * dt;

    if (camera.pos.y < floor) {
        camera.pos.y = floor;
        vert_speed = 0.0f;
    }
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
