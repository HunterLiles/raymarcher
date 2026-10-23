#version 460

#include "common.glsl"
#include "psrdnoise2.glsl"
#include "sdf.glsl"

#ifdef SDF_CACHE_UPDATE

void main() {
    uint group = gl_WorkGroupID.x + gl_WorkGroupID.y * gl_NumWorkGroups.x;
    uint i = group * gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z +
             gl_LocalInvocationIndex;

    if (i >= frame.count)
        return;

    float d = 0.0;

    switch (frame.id) {
#define SDF_CACHE_LIST_ONLY
#undef CACHE_SDF
#define CACHE_SDF(id, name, lower, upper, size, fps)                                               \
    case id:                                                                                       \
        d = name(engine_sdf_position(i, lower, upper, size));                                      \
        break;
#include "sdf.glsl"
#undef CACHE_SDF
#undef SDF_CACHE_LIST_ONLY

    default:
        return;
    }

    if (i == 0u)
        cache_storage.entries[frame.id] = uvec4(frame.offset, frame.count, 0u, 0u);

    cache_write(Cache(frame.offset, frame.count), i, vec4(d, 0.0, 0.0, 0.0));
}

#else

#include "hud.glsl"
#include "lighting.glsl"
#include "skybox.glsl"

vec3 raymarch(in vec3 ray_origin, in vec3 ray_dir) {
    float total_dist = 0.0f;
    const int MAX_STEPS = 512;
    const float MIN_HIT = 0.01f;
    const float MAX_DIST = 1000.0f;

    for (int i = 0; i < MAX_STEPS; ++i) {
        vec3 curr_pos = ray_origin + total_dist * ray_dir;

        Surface surface = map(curr_pos);

        if (surface.distance < MIN_HIT) {
            Material material = get_material(curr_pos, surface.material);

            float field_scale;
            vec3 normal = tangentNormal(curr_pos, surface.material, field_scale);

            vec3 emitter_pos = vec3(0.0, 1.0, 0.0);
            Material emitter = get_material(emitter_pos, 1);

            float fog = 1.0 / (1.0 + total_dist / 10.0);

            return color(curr_pos, normal, material, field_scale, emitter_pos, emitter.emission) *
                   fog;
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
    result = draw_debug_hud(result, pixel);

    ivec2 offset = abs(pixel - size / 2);
    bool crosshair = (offset.x <= 3 && offset.y == 0) || (offset.y <= 3 && offset.x == 0);

    if (crosshair)
        result = vec3(0.9);

    imageStore(image, pixel, vec4(result, 1.0));
}
#endif
