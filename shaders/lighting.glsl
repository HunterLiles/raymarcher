vec3 tangentNormal(vec3 p) {
    // Defining its nabla f (∇f) or gradient
    float e = 0.005;

    return normalize(vec3(map(p + vec3(e, 0, 0)).distance - map(p - vec3(e, 0, 0)).distance,
                          map(p + vec3(0, e, 0)).distance - map(p - vec3(0, e, 0)).distance,
                          map(p + vec3(0, 0, e)).distance - map(p - vec3(0, 0, e)).distance));
}

vec3 material_color(vec3 p, int material) {
    if (material == 1)
        return vec3(1.0, 0.03, 0.01);

    vec2 uv = p.xz;

    float patches = ground_noise(uv * 0.6);
    float detail = ground_noise(uv * 12.0);

    vec3 dirt = vec3(0.22, 0.13, 0.07);
    vec3 grass = vec3(0.16, 0.28, 0.08);

    float grass_amount = smoothstep(0.35, 0.65, patches);

    vec3 base = mix(dirt, grass, grass_amount);
    return base * mix(0.75, 1.15, detail);
}

vec3 color(vec3 obj, int material) {
    vec3 normal = tangentNormal(obj);

    vec3 sun_dir = sun_direction();
    vec3 moon_dir = -sun_dir;

    float day_light = daylight_amount(sun_dir);
    float sun_str = smoothstep(0.0, 0.15, sun_dir.y);

    float moon_str = smoothstep(0.0, 0.15, moon_dir.y);
    float moon_diffuse = max(dot(normal, moon_dir), 0.0);

    vec3 moon_light = vec3(0.08, 0.12, 0.22) * moon_diffuse * moon_str;

    vec3 sun_color =
        mix(vec3(1.0, 0.35, 0.12), vec3(1.0, 0.95, 0.85), smoothstep(0.0, 0.4, sun_dir.y));

    float diffuse = max(dot(normal, sun_dir), 0.0);

    vec3 ambient = mix(vec3(0.015, 0.02, 0.04), vec3(0.18, 0.20, 0.25), day_light);

    vec3 light = ambient + sun_color * diffuse * sun_str + moon_light;

    return material_color(obj, material) * light;
}
