#pragma once

#include "Vec3.hpp"
#include "Vec4.hpp"

#include <array>
#include <cmath>

// Row-major 4x4 matrix, column-vector convention (v' = M * v).
// Right-handed coordinates, camera looks down -Z, OpenGL-style clip space.
class Mat4 {
private:
    std::array<float, 16> data{};

public:
    static constexpr float PI = 3.14159265358979323846f;

    Mat4() = default;

    float& at(int row, int col) { return data[row * 4 + col]; }
    float at(int row, int col) const { return data[row * 4 + col]; }

    static float toRadians(float degrees) { return degrees * PI / 180.0f; }

    static Mat4 identity() {
        Mat4 r;
        for (int i = 0; i < 4; ++i) r.at(i, i) = 1.0f;
        return r;
    }

    static Mat4 translation(const Vec3& o) {
        Mat4 r = identity();
        r.at(0, 3) = o.x;
        r.at(1, 3) = o.y;
        r.at(2, 3) = o.z;
        return r;
    }

    static Mat4 scaling(const Vec3& f) {
        Mat4 r = identity();
        r.at(0, 0) = f.x;
        r.at(1, 1) = f.y;
        r.at(2, 2) = f.z;
        return r;
    }

    static Mat4 rotationX(float degrees) {
        const float a = toRadians(degrees), c = std::cos(a), s = std::sin(a);
        Mat4 r = identity();
        r.at(1, 1) = c;  r.at(1, 2) = -s;
        r.at(2, 1) = s;  r.at(2, 2) = c;
        return r;
    }

    static Mat4 rotationY(float degrees) {
        const float a = toRadians(degrees), c = std::cos(a), s = std::sin(a);
        Mat4 r = identity();
        r.at(0, 0) = c;  r.at(0, 2) = s;
        r.at(2, 0) = -s; r.at(2, 2) = c;
        return r;
    }

    static Mat4 rotationZ(float degrees) {
        const float a = toRadians(degrees), c = std::cos(a), s = std::sin(a);
        Mat4 r = identity();
        r.at(0, 0) = c;  r.at(0, 1) = -s;
        r.at(1, 0) = s;  r.at(1, 1) = c;
        return r;
    }

    // Maps view space to clip space. After the perspective divide x,y,z are in [-1, 1].
    static Mat4 perspective(float fovDegrees, float aspect, float nearPlane, float farPlane) {
        const float f = 1.0f / std::tan(toRadians(fovDegrees) * 0.5f);
        Mat4 r;
        r.at(0, 0) = f / aspect;
        r.at(1, 1) = f;
        r.at(2, 2) = (farPlane + nearPlane) / (nearPlane - farPlane);
        r.at(2, 3) = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
        r.at(3, 2) = -1.0f;
        return r;
    }

    Vec3 transformPoint(const Vec3& point) const {
        float x =
            at(0, 0) * point.x +
            at(0, 1) * point.y +
            at(0, 2) * point.z +
            at(0, 3);

        float y =
            at(1, 0) * point.x +
            at(1, 1) * point.y +
            at(1, 2) * point.z +
            at(1, 3);

        float z =
            at(2, 0) * point.x +
            at(2, 1) * point.y +
            at(2, 2) * point.z +
            at(2, 3);

        float w =
            at(3, 0) * point.x +
            at(3, 1) * point.y +
            at(3, 2) * point.z +
            at(3, 3);

        if (w != 0.0f && w != 1.0f) {
            x /= w;
            y /= w;
            z /= w;
        }

        return Vec3(x, y, z);
    }
};
