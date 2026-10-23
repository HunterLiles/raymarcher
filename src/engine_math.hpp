#pragma once

#include <cmath>

struct vec2 {
    float x{}, y{};
};

struct vec3 {
    float x{}, y{}, z{};

    vec3 operator+(vec3 v) const {
        return {x + v.x, y + v.y, z + v.z};
    }
    vec3 operator-(vec3 v) const {
        return {x - v.x, y - v.y, z - v.z};
    }
    vec3 operator*(float s) const {
        return {x * s, y * s, z * s};
    }
    vec3 operator/(float s) const {
        return {x / s, y / s, z / s};
    }

    vec3 &operator+=(vec3 v) {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }
    vec3 &operator-=(vec3 v) {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }
};

struct vec4 {
    float x{}, y{}, z{}, w{};
};

struct mat3 {
    vec3 mat[3];
};

struct mat4 {
    vec4 mat[4];
};

struct quaternion {
    float x{}, y{}, z{}, w{1.0f};

    quaternion operator*(quaternion q) const {
        return {w * q.x + x * q.w + y * q.z - z * q.y, w * q.y - x * q.z + y * q.w + z * q.x,
                w * q.z + x * q.y - y * q.x + z * q.w, w * q.w - x * q.x - y * q.y - z * q.z};
    }
};

inline float dot(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline vec3 cross(vec3 a, vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline vec3 normalize(vec3 v) {
    float length = std::sqrt(dot(v, v));
    return length > 0.0f ? v / length : vec3{};
}

inline quaternion normalize(quaternion q) {
    float length = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (length == 0.0f)
        return {};
    return {q.x / length, q.y / length, q.z / length, q.w / length};
}

inline quaternion axis_angle(vec3 axis, float angle) {
    float length = std::sqrt(dot(axis, axis));
    if (length == 0.0f)
        return {};

    float scale = std::sin(angle * 0.5f) / length;
    return {axis.x * scale, axis.y * scale, axis.z * scale, std::cos(angle * 0.5f)};
}

inline vec3 rotate(quaternion q, vec3 v) {
    vec3 axis{q.x, q.y, q.z};
    vec3 t = cross(axis, v) * 2.0f;
    return v + t * q.w + cross(axis, t);
}
