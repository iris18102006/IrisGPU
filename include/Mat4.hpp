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

    static Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
        const Vec3 f = normalize(target - eye);
        const Vec3 s = normalize(cross(f, up));
        const Vec3 u = cross(s, f);

        Mat4 r = identity();
        r.at(0, 0) = s.x;  r.at(0, 1) = s.y;  r.at(0, 2) = s.z;  r.at(0, 3) = -dot(s, eye);
        r.at(1, 0) = u.x;  r.at(1, 1) = u.y;  r.at(1, 2) = u.z;  r.at(1, 3) = -dot(u, eye);
        r.at(2, 0) = -f.x; r.at(2, 1) = -f.y; r.at(2, 2) = -f.z; r.at(2, 3) = dot(f, eye);
        return r;
    }

    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int row = 0; row < 4; ++row)
            for (int col = 0; col < 4; ++col)
                for (int k = 0; k < 4; ++k)
                    r.at(row, col) += at(row, k) * o.at(k, col);
        return r;
    }

    Vec4 operator*(const Vec4& v) const {
        return {
            at(0, 0) * v.x + at(0, 1) * v.y + at(0, 2) * v.z + at(0, 3) * v.w,
            at(1, 0) * v.x + at(1, 1) * v.y + at(1, 2) * v.z + at(1, 3) * v.w,
            at(2, 0) * v.x + at(2, 1) * v.y + at(2, 2) * v.z + at(2, 3) * v.w,
            at(3, 0) * v.x + at(3, 1) * v.y + at(3, 2) * v.z + at(3, 3) * v.w
        };
    }

    // Affine transform of a point (w = 1). No perspective divide here on purpose.
    Vec3 transformPoint(const Vec3& p) const { return (*this * Vec4(p, 1.0f)).xyz(); }

    // Transform of a direction (w = 0): ignores translation.
    Vec3 transformDirection(const Vec3& d) const { return (*this * Vec4(d, 0.0f)).xyz(); }
};
