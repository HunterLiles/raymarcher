#version 460
#extension GL_GOOGLE_include_directive : require

layout(push_constant) uniform Frame {
    vec2 resolution;
    float time;
} frame;

layout(location = 0) out vec4 out_color;

#include "sdf.glsl"
#include "lighting.glsl"

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
    // Center the pixels, preserve aspect ratio, and point +Y upward.
    vec2 uv = (2.0 * gl_FragCoord.xy - frame.resolution) / frame.resolution.y;
    uv.y = -uv.y;

    vec3 ray_origin = vec3(0.0, 0.0, -5.0);
    vec3 ray_dir = normalize(vec3(uv, 1.0));

    vec3 image = ray_march(ray_origin, ray_dir);

    // Output to screen
    out_color = vec4(image, 1.0);
}
