#pragma once
#include <cmath>
namespace math {
struct Vec3 {
    float x{}, y{}, z{};
};
struct Vec4 {
    float x{}, y{}, z{}, w{};
};
// Column-major storage, column vectors, right-handed world, Vulkan depth [0, 1].
struct Mat4 {
    float e[16]{};
};
inline Vec3 add(Vec3 a, Vec3 b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
inline Vec3 sub(Vec3 a, Vec3 b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
inline Vec3 scale(Vec3 a, float s) {
    return {a.x * s, a.y * s, a.z * s};
}
inline float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline Vec3 normalize(Vec3 a) {
    float n = std::sqrt(dot(a, a));
    return n > 0 ? scale(a, 1 / n) : Vec3{};
}
inline Mat4 identity() {
    Mat4 m{};
    m.e[0] = m.e[5] = m.e[10] = m.e[15] = 1;
    return m;
}
inline Mat4 multiply(const Mat4 &a, const Mat4 &b) {
    Mat4 r{};
    for (int c = 0; c < 4; ++c)
        for (int row = 0; row < 4; ++row)
            for (int k = 0; k < 4; ++k)
                r.e[c * 4 + row] += a.e[k * 4 + row] * b.e[c * 4 + k];
    return r;
}
inline Vec4 transform(const Mat4 &m, Vec4 v) {
    return {m.e[0] * v.x + m.e[4] * v.y + m.e[8] * v.z + m.e[12] * v.w,
            m.e[1] * v.x + m.e[5] * v.y + m.e[9] * v.z + m.e[13] * v.w,
            m.e[2] * v.x + m.e[6] * v.y + m.e[10] * v.z + m.e[14] * v.w,
            m.e[3] * v.x + m.e[7] * v.y + m.e[11] * v.z + m.e[15] * v.w};
}
inline Mat4 translation(Vec3 p) {
    auto m = identity();
    m.e[12] = p.x;
    m.e[13] = p.y;
    m.e[14] = p.z;
    return m;
}
inline Mat4 rotation_y(float a) {
    auto m = identity();
    float c = std::cos(a), s = std::sin(a);
    m.e[0] = c;
    m.e[2] = -s;
    m.e[8] = s;
    m.e[10] = c;
    return m;
}
inline Mat4 perspective(float fov, float aspect, float near_z, float far_z) {
    Mat4 m{};
    float f = 1 / std::tan(fov * .5f);
    m.e[0] = f / aspect;
    m.e[5] = -f;
    m.e[10] = far_z / (near_z - far_z);
    m.e[11] = -1;
    m.e[14] = far_z * near_z / (near_z - far_z);
    return m;
}
inline Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up) {
    Vec3 f = normalize(sub(target, eye)), r = normalize(cross(f, up)), u = cross(r, f);
    auto m = identity();
    m.e[0] = r.x;
    m.e[4] = r.y;
    m.e[8] = r.z;
    m.e[1] = u.x;
    m.e[5] = u.y;
    m.e[9] = u.z;
    m.e[2] = -f.x;
    m.e[6] = -f.y;
    m.e[10] = -f.z;
    m.e[12] = -dot(r, eye);
    m.e[13] = -dot(u, eye);
    m.e[14] = dot(f, eye);
    return m;
}
} // namespace math
