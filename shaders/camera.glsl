vec3 rotate(vec4 orientation, vec3 direction) {
    vec3 t = 2.0 * cross(orientation.xyz, direction);
    return direction + orientation.w * t + cross(orientation.xyz, t);
}

vec3 camera_dir(vec2 uv, vec4 orientation) {
    return rotate(orientation, normalize(vec3(uv, 1.0)));
}
