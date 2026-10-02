float sphere(in vec3 p, in vec3 c, in float r) {
    return length(p - c) - r;
}

float map(in vec3 p) {
    float cos_dis = cos(p.x * 4.0) * cos(p.y * 2.0) * cos(p.z * 2.0) * cos(iTime);
    float sin_dis = sin(p.x * 2.0) * sin(p.y * 3.0) * sin(p.z * 4.0) * sin(iTime);
    float sphere = sphere(p, vec3(0.0, 0.0, 0.0), 1.0);
    return sphere;
}
