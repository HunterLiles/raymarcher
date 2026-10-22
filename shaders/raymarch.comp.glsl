#version 460

layout(local_size_x = 8, local_size_y = 8) in;
layout(set = 0, binding = 0, rgba8) uniform writeonly image2D image;

layout(push_constant) uniform Frame {
    vec2 resolution;
    float time;
}

frame;

#include "sdf.glsl"

#include "lighting.glsl"

vec3 raymarch(in vec3 ray_origin, in vec3 ray_dir) {
    float total_dist = 0.0f;
    const int MAX_STEPS = 128;
    const float MIN_HIT = 0.0001f;
    const float MAX_DIST = 1000.0f;

    for (int i = 0; i < MAX_STEPS; ++i) {
        vec3 curr_pos = ray_origin + total_dist * ray_dir;

        float closest_dist = map(curr_pos);

        if (closest_dist < MIN_HIT) {
            return color(curr_pos);
        }

        if (total_dist > MAX_DIST) {
            break;
        }

        total_dist += closest_dist;
    }

    return vec3(0.0f);
}

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(image);

    if (any(greaterThanEqual(pixel, size)))
        return;

    vec2 uv = (2.0 * (vec2(pixel) + 0.5) - vec2(size)) / vec2(size.y);
    uv.y = -uv.y;

    vec3 ray_origin = vec3(0.0, 0.0, -5.0);
    vec3 ray_dir = normalize(vec3(uv, 1.0));
    vec3 result = raymarch(ray_origin, ray_dir);

    imageStore(image, pixel, vec4(result, 1.0));
}
