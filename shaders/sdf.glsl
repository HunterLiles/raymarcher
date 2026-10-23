#define CACHE_FIRE 0

#ifndef SDF_CACHE_LIST_ONLY
#include "common.glsl"

struct Surface {
    float distance;
    int material;
};

// turn scene into whatever the closest thing is
void add_surface(inout Surface scene, float distance, int material) {
    if (distance < scene.distance)
        scene = Surface(distance, material);
}

float fire(vec3 p) {
    const float fire_height = 1.0;

    float h = clamp(p.y / fire_height, 0.0, 1.0);
    float t = frame.time;

    vec2 bend = vec2(0.1 * sin(p.y * 2.5 - t * 1.7), 0.1 * sin(p.y * 3.1 - t * 1.3 + 2.0));

    float radius = (1.0 - h) * (0.4 + 0.08 * sin(p.y * 9.0 - t * 5.0));

    float side = length(p.xz + bend) - radius;
    float caps = max(-p.y, p.y - fire_height);

    return max(side, caps) / 3.0;
}
#endif

CACHE_SDF(CACHE_FIRE, fire, vec3(-1.0, 0.0, -1.0), vec3(1.0, 1.0, 1.0), ivec3(32, 64, 32), 8.0)

#ifndef SDF_CACHE_LIST_ONLY

// I want this to be redone
const vec2 terrain_frequency = vec2(0.25, 0.9);
const vec2 terrain_amplitude = vec2(0.225, 0.04);
const float terrain_slope_bound = 13.4 * dot(terrain_amplitude, terrain_frequency);
const float terrain_distance_scale = inversesqrt(1.0 + terrain_slope_bound * terrain_slope_bound);

float terrain_height(vec2 p, out vec2 gradient) {
    vec2 hills_gradient, detail_gradient;

    float hills = simplex_noise(p * terrain_frequency.x, hills_gradient);

    float detail = simplex_noise(p * terrain_frequency.y, detail_gradient);

    gradient = hills_gradient * (terrain_amplitude.x * terrain_frequency.x) +
               detail_gradient * (terrain_amplitude.y * terrain_frequency.y);

    return dot(vec2(hills, detail), terrain_amplitude);
}

float terrain_height(vec2 p) {
    vec2 gradient;
    return terrain_height(p, gradient);
}

Surface map(in vec3 p) {

    Surface scene = Surface(1e20, -1);

    add_surface(scene, cached_fire(p, vec3(0.0), 1.0), 1);
    add_surface(scene, (p.y - terrain_height(p.xz)) * terrain_distance_scale, 0);

    return scene;
}

#endif
