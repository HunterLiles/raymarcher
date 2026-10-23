#include "scripts/controller.hpp"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <engine/engine.hpp>
#include <exception>

int main() {
    try {
        engine::Engine engine({
            .title = "Medieval Fantasy",
            .pixel_size = 2,
            .shader = GAME_SOURCE_DIR "/shaders/raymarch.comp.glsl",
            .sdf = GAME_SOURCE_DIR "/shaders/sdf.glsl",
            .compiler = GAME_GLSLC,
            .hot_reload = true,
        });

        engine::Camera camera{};
        Controller controller{};

        glfwSetInputMode(engine.window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        while (engine.is_running()) {
            if (glfwGetKey(engine.window(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
                break;

            controller.camera_rotate(camera, engine.window());
            controller.camera_move(camera, engine.dt(), engine.window());

            engine.draw(camera);
        }

    } catch (const std::exception &e) {
        std::fprintf(stderr, "Fatal: %s\n", e.what());
        return 1;
    }
}
