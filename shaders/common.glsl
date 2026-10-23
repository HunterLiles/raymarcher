#ifndef COMMON_GLSL
#define COMMON_GLSL

layout(local_size_x = 8, local_size_y = 8) in;
layout(set = 0, binding = 0, rgba8) uniform writeonly image2D image;

layout(push_constant, std430) uniform Frame {
#define GPU_FIELD(glsl, cpp, name) glsl name;
#include "../engine/frame_fields.inc"
#undef GPU_FIELD
}
frame;

#include "../engine/cache.glsl"

float daylight_amount(vec3 sun_direction) {
    return smoothstep(-0.15, 0.15, sun_direction.y);
}

vec3 sun_direction() {
    const float day_length = 120.0;
    float angle = frame.time * 6.2831 / day_length;

    return normalize(vec3(cos(angle), sin(angle), 0.3));
}

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
