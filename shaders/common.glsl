#ifndef COMMON_GLSL
#define COMMON_GLSL

layout(local_size_x = 8, local_size_y = 8) in;
layout(set = 0, binding = 0, rgba8) uniform writeonly image2D image;

layout(push_constant) uniform Frame {
    vec4 camera_rot;
    vec4 camera_pos;
    vec2 resolution;
    float time;
    vec4 metrics;
}
frame;

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
#endif
