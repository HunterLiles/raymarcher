#version 460

#include "sdf.glsl"

#include "camera.glsl"
#include "common.glsl"
#include "hud.glsl"
#include "lighting.glsl"
#include "post.glsl"
#include "sky.glsl"

vec3 raymarch(in vec3 ray_origin, in vec3 ray_dir) {
    float total_dist = 0.0f;
    const int MAX_STEPS = 256;
    const float MIN_HIT = 0.01f;
    const float MAX_DIST = 1000.0f;

    for (int i = 0; i < MAX_STEPS; ++i) {
        vec3 curr_pos = ray_origin + total_dist * ray_dir;

        Surface surface = map(curr_pos);

        if (surface.distance < MIN_HIT) {
            float fog = 1.0 / (1.0 + total_dist / 10.0);
            return color(curr_pos, surface.material) * fog;
        }

        if (total_dist > MAX_DIST) {
            break;
        }

        total_dist += surface.distance;
    }

    return sky(ray_dir);
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
    result = draw_hud(result, pixel);

    ivec2 offset = abs(pixel - size / 2);
    bool crosshair = (offset.x <= 3 && offset.y == 0) || (offset.y <= 3 && offset.x == 0);

    if (crosshair)
        result = vec3(0.9);

    imageStore(image, pixel, vec4(result, 1.0));
}
