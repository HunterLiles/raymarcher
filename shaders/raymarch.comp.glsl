#version 460

#include "sdf.glsl"

#include "camera.glsl"
#include "lighting.glsl"
#include "post.glsl"

vec3 raymarch(in vec3 ray_origin, in vec3 ray_dir) {
    float total_dist = 0.0f;
    const int MAX_STEPS = 128;
    const float MIN_HIT = 0.0001f;
    const float MAX_DIST = 1000.0f;

    for (int i = 0; i < MAX_STEPS; ++i) {
        vec3 curr_pos = ray_origin + total_dist * ray_dir;

        float closest_dist = map(curr_pos);

        if (closest_dist < MIN_HIT) {
            float fog = 1.0 / (1.0 + total_dist / 10.0);
            return color(curr_pos) * fog;
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

    vec2 uv = 2.0 * (vec2(pixel) + 0.5) / vec2(size) - 1.0;
    uv.x *= frame.resolution.x / frame.resolution.y;
    uv.y = -uv.y;

    vec3 ray_origin = frame.camera_pos.xyz;
    vec3 ray_dir = camera_dir(uv, frame.camera_rot);

    vec3 result = raymarch(ray_origin, ray_dir);
    result = psx_color(result, pixel);
    imageStore(image, pixel, vec4(result, 1.0));
}
