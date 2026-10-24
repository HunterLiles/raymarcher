#include "skybox.glsl"

// PBR
struct Material {
    vec3 albedo;
    vec3 emission;
};

Material get_material(vec3 p, int material) {
    if (material == 1)
        return Material(vec3(1.0, 0.03, 0.01), vec3(1.0, 0.15, 0.02) * 2.0);

    vec2 uv = p.xz;

    float patches = ground_noise(uv * 0.6);
    float detail = ground_noise(uv * 12.0);

    vec3 dirt = vec3(0.22, 0.13, 0.07);
    vec3 grass = vec3(0.16, 0.28, 0.08);

    float grass_amount = smoothstep(0.35, 0.65, patches);

    vec3 base = mix(dirt, grass, grass_amount);
    return Material(base * mix(0.75, 1.15, detail), vec3(0.0));
}

// Normal of the object for lighting using gradients
vec3 tangentNormal(vec3 p, out float field_scale) {
    // Defining its nabla f (∇f) or gradient
    float e = 0.005;

    vec3 gradient = vec3(map(p + vec3(e, 0, 0)).distance - map(p - vec3(e, 0, 0)).distance,
                         map(p + vec3(0, e, 0)).distance - map(p - vec3(0, e, 0)).distance,
                         map(p + vec3(0, 0, e)).distance - map(p - vec3(0, 0, e)).distance) /
                    (2.0 * e);

    field_scale = max(length(gradient), 0.000001);
    return gradient / field_scale;
}

float AO(vec3 p, vec3 normal, float field_scale) {
    float occlusion = 0.0;
    float weight = 10.0;
    float height = 0.1;

    for (int i = 0; i < 3; ++i) {
        float d = map(p + normal * height).distance / field_scale;

        occlusion += weight * clamp(1.0 - d / height, 0.0, 1.0);

        height *= 2.0;
        weight *= 0.5;
    }

    return clamp(1.0 - occlusion / 1.75, 0.0, 1.0);
}

// Send out a second ray from the object to the light source
float shadow(vec3 origin, vec3 dir) {
    float dist = 0.01;
    float visibility = 1.0;

    const int max_steps = 64;
    const float max_dist = 50.0;
    const float min_hit = 0.001;
    const float sharp = 16.0;

    for (int i = 0; i < max_steps; ++i) {
        if (dist >= max_dist)
            break;

        float d = map(origin + dir * dist).distance;

        if (d < min_hit)
            return 0.0;

        visibility = min(visibility, sharp * d / dist);
        dist += d;
    }

    return clamp(visibility, 0.0, 1.0);
} // returns value 0 -> 1 defining how visible it is

vec3 point_light(vec3 p, vec3 normal, vec3 pos, vec3 emission, float power) {
    vec3 offset = pos - p;
    float dist_squared = max(dot(offset, offset), 0.0001);
    vec3 dir = offset * inversesqrt(dist_squared);

    float diffuse = max(dot(normal, dir), 0.0);
    float attenuation = power / (1.0 + dist_squared);

    return emission * diffuse * attenuation;
}

vec2 surface_shadow(vec3 p) {
    float field_scale;
    vec3 normal = tangentNormal(p, field_scale);
    vec3 light_dir = sky_light_direction();

    float visibility = 1.0;
    if (dot(normal, light_dir) <= 0.0 || light_dir.y <= 0.0)
        visibility = shadow(p + normal * 0.02, light_dir);

    return vec2(visibility, AO(p, normal, field_scale));
}

vec2 shadow_visibility(ivec2 pixel) {
    ivec2 size = imageSize(image);
    ivec2 shadow_size = imageSize(shadow_image);
    ivec2 p = min(ivec2((vec2(pixel) + 0.5) * vec2(shadow_size) / vec2(size)), shadow_size - 1);

    return imageLoad(shadow_image, p).rg;
}

vec3 color(vec3 obj, int material_id, float dist, ivec2 pixel) {
    float field_scale;
    vec3 normal = tangentNormal(obj, field_scale);
    Material material = get_material(obj, material_id);

    vec2 visbility = shadow_visibility(pixel);
    vec3 light = sky_light(normal, visbility.y, visbility.x);

    vec3 emitter_pos = vec3(0.0, 1.0, 0.0);
    Material emitter = get_material(emitter_pos, 1);
    light += point_light(obj, normal, emitter_pos, emitter.emission, 4.0);

    float fog = 1.0 / (1.0 + dist / 10.0);
    return (material.albedo * light + material.emission) * fog;
}

// PSX dithering
vec3 psx_color(vec3 rgb, ivec2 pixel) {
    const float bayer[16] = float[16](0.0, 8.0, 2.0, 10.0, 12.0, 4.0, 14.0, 6.0, 3.0, 11.0, 1.0,
                                      9.0, 15.0, 7.0, 13.0, 5.0);

    int index = (pixel.y % 4) * 4 + (pixel.x % 4);
    float threshold = (bayer[index] + 0.5) / 16.0;

    const float levels = 32.0;
    rgb = clamp(rgb, 0.0, 1.0);

    return floor(rgb * (levels - 1.0) + threshold) / (levels - 1.0);
}
