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
            .shadow_pixel_size = 8,
            .hot_reload = true,
            .shader = GAME_SOURCE_DIR "/shaders/raymarch.comp.glsl",
            .sdf = GAME_SOURCE_DIR "/shaders/sdf.glsl",
            .compiler = GAME_GLSLC,
        });

        Camera camera{};
        Controller controller{};

        glfwSetInputMode(engine.window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        while (engine.is_running()) {
            if (glfwGetKey(engine.window(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
                break;

            controller.camera_rotate(camera, engine.window());
            controller.camera_move(camera, engine.dt(), engine.window());

            const auto &player = controller.player;

            // Camera
            engine.set_uniform("camera_rot", camera.orientation);
            engine.set_uniform("camera_pos", engine::vec4{
                                                 camera.pos.x,
                                                 camera.pos.y,
                                                 camera.pos.z,
                                                 0.0f,
                                             });

            // Hud percentages
            engine.set_uniform("health", player.health.x / player.health.y);
            engine.set_uniform("mana", player.mana.x / player.mana.y);
            engine.set_uniform("stamina", player.stamina.x / player.stamina.y);

            // Hud debug metrics
            const auto stats = engine.stats();
            const float ms = engine.dt() * 1000.0f;
            engine.set_uniform("metrics", engine::vec4{
                                              ms > 0.0f ? 1000.0f / ms : 0.0f,
                                              ms,
                                              float(stats.cpu_work_ms),
                                              stats.gpu_available ? float(stats.gpu_ms) : -1.0f,
                                          });

            engine.draw();
        }

    } catch (const std::exception &e) {
        std::fprintf(stderr, "Fatal: %s\n", e.what());
        return 1;
    }
}
