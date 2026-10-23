#include "common.glsl"

struct Surface {
    float distance;
    int material;
};

float sphere(in vec3 p, in vec3 c, in float r) {
    return length(p - c) - r;
}

float zx_plane(in vec3 p) {
    return p.y;
}

float fire(vec3 p, vec3 pos, float scale) {
    vec3 q = (p - pos) / scale;

    const float height = 3.0;
    float h = clamp(q.y / height, 0.0, 1.0);
    float t = frame.time;

    q.x += 0.10 * sin(q.y * 2.5 - t * 1.7);
    q.z += 0.10 * sin(q.y * 3.1 - t * 1.3 + 2.0);

    float radius = 0.4 * (1.0 - h);

    radius += 0.08 * (1.0 - h) * sin(q.y * 9.0 - t * 5.0);

    float side = length(q.xz) - radius;
    float caps = max(-q.y, q.y - height);

    return max(side, caps) * (scale / 3.0);
}

float ground_noise(vec2 p) {
    vec2 cell = floor(p);
    vec2 f = fract(p);

    vec2 weight = f * f * (3.0 - 2.0 * f);

    float a = hash(cell);
    float b = hash(cell + vec2(1.0, 0.0));
    float c = hash(cell + vec2(0.0, 1.0));
    float d = hash(cell + vec2(1.0, 1.0));

    return mix(mix(a, b, weight.x), mix(c, d, weight.x), weight.y);
}

float terrain_height(vec2 p) {
    float hills = (ground_noise(p * 0.25) - 0.5) * 0.45;
    float detail = (ground_noise(p * 0.9) - 0.5) * 0.08;

    return hills + detail;
}

Surface map(in vec3 p) {
    Surface flame = Surface(fire(p, vec3(0.0), 1.0), 1);
    float ground_dist = (p.y - terrain_height(p.xz)) / 1.1;
    Surface ground = Surface(ground_dist, 0);

    return flame.distance < ground.distance ? flame : ground;
}
