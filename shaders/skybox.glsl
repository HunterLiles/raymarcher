#include "common.glsl"
#include "psrdnoise2.glsl"

float daylight_amount(vec3 sun_direction) {
    return smoothstep(-0.15, 0.15, sun_direction.y);
}

vec3 sun_direction() {
    const float day_length = 120.0;
    float angle = frame.time * 6.2831 / day_length;

    return normalize(vec3(cos(angle), sin(angle), 0.3));
}

vec3 sky_light_direction() {
    vec3 sun = sun_direction();
    return sun.y >= 0.0 ? sun : -sun;
}

vec3 sky_light(vec3 normal, float occlusion, float visibility) {
    vec3 sun = sun_direction();

    bool isDay = sun.y >= 0.0;
    float day_light = daylight_amount(sun);
    float strength = smoothstep(0.0, 0.15, abs(sun.y));

    vec3 light_dir = isDay ? sun : -sun;
    float diffuse = max(dot(normal, light_dir), 0.0);

    vec3 dir_color =
        isDay ? mix(vec3(1.0, 0.35, 0.12), vec3(1.0, 0.95, 0.85), smoothstep(0.0, 0.4, sun.y))
              : vec3(0.08, 0.12, 0.22) * 5.0;

    vec3 sky_ambient = mix(vec3(0.06, 0.08, 0.12), vec3(0.18, 0.2, 0.25), day_light);
    vec3 ground_ambient = sky_ambient * vec3(0.45, 0.35, 0.25);

    vec3 ambient = mix(ground_ambient, sky_ambient, normal.y * 0.5 + 0.5);
    ambient *= occlusion;

    return ambient + dir_color * diffuse * strength * visibility;
}

float stars(vec3 direction) {
    const float pi = 3.14159265;

    vec2 uv = vec2(atan(direction.z, direction.x) / (2.0 * pi) + 0.5,
                   asin(clamp(direction.y, -1.0, 1.0)) / pi + 0.5);

    vec2 grid = uv * vec2(180.0, 90.0);
    vec2 cell = floor(grid);
    vec2 local = fract(grid);

    float seed = hash(cell);

    vec2 center = mix(vec2(0.25), vec2(0.75), vec2(hash(cell + 17.0), hash(cell + 53.0)));

    float shape = 1.0 - smoothstep(0.05, 0.20, length(local - center));

    return shape * step(0.985, seed) * mix(0.5, 1.0, seed);
}

vec3 sky(vec3 direction) {
    direction = normalize(direction);

    vec3 sun_dir = sun_direction();
    float sun_dot = dot(direction, sun_dir);

    float daylight = daylight_amount(sun_dir);
    float night = 1.0 - daylight;
    float elevation = sqrt(clamp(direction.y, 0.0, 1.0));

    vec3 day_sky = mix(vec3(0.65, 0.55, 0.60), vec3(0.12, 0.20, 0.38), elevation);
    vec3 night_sky = mix(vec3(0.025, 0.025, 0.065), vec3(0.003, 0.005, 0.020), elevation);

    float visibility = smoothstep(0.0, 0.2, direction.y);

    if (night > 0.0 && visibility > 0.0) {
        vec3 galaxy_axis = normalize(vec3(0.3, 0.8, 0.5));
        float band_distance = abs(dot(direction, galaxy_axis));
        float band = 1.0 - smoothstep(0.02, 0.22, band_distance);

        float clouds = ground_noise(direction.xz * 8.0 + direction.y * 3.0);
        vec3 galaxy = mix(vec3(0.06, 0.08, 0.18), vec3(0.22, 0.10, 0.25), clouds);

        night_sky += galaxy * band * clouds;
        night_sky += vec3(0.8, 0.85, 1.0) * stars(direction) * visibility;
    }

    vec3 result = mix(night_sky, day_sky, daylight);

    float sun = step(cos(0.045), sun_dot);
    sun *= smoothstep(-0.05, 0.05, sun_dir.y);
    result = mix(result, vec3(1.0, 0.85, 0.55), sun);

    float moon = step(cos(0.035), -sun_dot);
    moon *= smoothstep(0.0, 0.05, -sun_dir.y) * night;
    result = mix(result, vec3(0.65, 0.75, 0.95), moon);

    return result;
}
