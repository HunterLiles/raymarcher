vec3 tangentNormal(vec3 p) {
    // Defining its partials along x and y
    float e = 0.005;

    return normalize(vec3(map(p + vec3(e, 0, 0)) - map(p - vec3(e, 0, 0)),
                          map(p + vec3(0, e, 0)) - map(p - vec3(0, e, 0)),
                          map(p + vec3(0, 0, e)) - map(p - vec3(0, 0, e))));
}

vec3 color(vec3 obj) {
    vec3 normal = tangentNormal(obj);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diffuse = max(dot(normal, lightDir), 0.0);

    float light = 0.3 + 0.7 * diffuse;

    float h = smoothstep(-0.45, 0.45, obj.z);
    float strength = clamp(0.05 + 0.55 * h + 0.5 * diffuse, 0.0, 1.0);

    return mix(vec3(0.125, 0.157, 0.243), vec3(0.769, 0.639, 0.353), strength);
}
