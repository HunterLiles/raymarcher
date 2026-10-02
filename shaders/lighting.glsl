vec3 calc_norm(in vec3 p) {
    const vec3 small_step = vec3(0.001f, 0.0f, 0.0f);

    float gradient_x = map(p + small_step.xyy) - map(p - small_step.xyy);
    float gradient_y = map(p + small_step.yxy) - map(p - small_step.yxy);
    float gradient_z = map(p + small_step.yyx) - map(p - small_step.yyx);

    vec3 normal = vec3(gradient_x, gradient_y, gradient_z);

    return normalize(normal);
}

vec3 lighting(in vec3 p) {

    vec3 normal = calc_norm(p);
    vec3 light_pos = vec3(sin(iTime), 3.0, cos(iTime));
    vec3 dir_to_light = normalize(light_pos);

    float diffuse_intensity = max(0.0f, length(normal * dir_to_light));
    return normal * diffuse_intensity;
}
