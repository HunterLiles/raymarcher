float sphere(in vec3 p, in vec3 c, in float r) {
    return length(p - c) - r;
}

float map(in vec3 p) {
    return sphere(p, vec3(0.0), 1.0);
}
