#version 460

#include "lighting.glsl"
#include "sdf.glsl"

layout(location = 0) out vec4 out_color;

in vec2 fragCoord;

vec3 ray_march(in vec3 ray_origin, in vec3 ray_dir) {
    float total_dist = 0.0f;
    const int MAX_STEPS = 128;
    const float MIN_HIT = 0.000001f;
    const float MAX_DIST = 1000.0f;

    for (int i = 0; i < MAX_STEPS; ++i) {
        vec3 curr_pos = ray_origin + total_dist * ray_dir;

        float closest_dist = map(curr_pos);

        if (closest_dist < MIN_HIT) {
            return lighting(curr_pos);
        }

        if (total_dist > MAX_DIST) {
            break;
        }

        total_dist += closest_dist;
    }

    return vec3(0.0f);
}

void main() {
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord / iResolution.xy * 2.0f - 1.0f;

    vec3 camera_p = vec3(0.0f, 0.0f, -5.0f);
    vec3 ray_origin = camera_p;
    vec3 ray_dir = vec3(uv, 1.0f);

    vec3 image = ray_march(ray_origin, ray_dir);

    // Output to screen
    fragColor = vec4(image, 1.0);
}
