#include "common.glsl"

float sphere(in vec3 p, in vec3 c, in float r) {
    return length(p - c) - r;
}

float xy_plane(in vec3 p) {
    return p.z;
}

float zy_plane(in vec3 p) {
    return p.x;
}

float zx_plane(in vec3 p) {
    return p.y;
}

float repeated_spheres(in vec3 p) {
    const float spacing = 0.6;
    const float speed = 0.8;

    vec3 local = p;
    local.y -= frame.time * speed;
    local.y = mod(local.y + spacing * 0.5, spacing) - spacing * 0.5;

    return sphere(local, vec3(0.0), 0.4);
}

float map(in vec3 p) {
    float fire = repeated_spheres(p);
    float ground = zx_plane(p) + 1.0;

    return min(fire, ground);
}
