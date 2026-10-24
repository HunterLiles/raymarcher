#ifndef COMMON_GLSL
#define COMMON_GLSL

layout(local_size_x = 8, local_size_y = 8) in;
layout(set = 0, binding = 0, rgba8) uniform writeonly image2D image;
layout(set = 0, binding = 3, rgba8) uniform image2D shadow_image;

#ifdef SDF_CACHE_UPDATE
layout(push_constant) uniform CacheFrame {
    float time;
    uint offset;
    uint count;
    uint id;
}
frame;
#else

layout(push_constant, std430) uniform Frame {
#define GPU_FIELD(glsl, cpp, name) glsl name;
#include "../engine/frame_fields.inc"
#undef GPU_FIELD
}
frame;
#endif

layout(set = 0, binding = 2, std140) uniform GameUniforms {
    vec4 camera_rot;
    vec4 camera_pos;
    vec4 metrics;
    float health;
    float mana;
    float stamina;
}
game;

#include "../engine/cache.glsl"

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// Camera
vec3 rotate(vec4 orientation, vec3 direction) {
    vec3 t = 2.0 * cross(orientation.xyz, direction);
    return direction + orientation.w * t + cross(orientation.xyz, t);
}

vec3 camera_dir(vec2 uv, vec4 orientation) {
    return rotate(orientation, normalize(vec3(uv, 1.0)));
}
#endif
