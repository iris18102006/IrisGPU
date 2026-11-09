#pragma once

#include "Vec3.hpp"
#include <array>
#include <cmath>

class Mat4 {
private:
    std::array<float, 16> data{};

public:
    Mat4() = default;

    float& at(int row, int col) {
        return data[row * 4 + col];
    }

    float at(int row, int col) const {
        return data[row * 4 + col];
    }

    static Mat4 identity() {
        Mat4 result;

        for (int i = 0; i < 4; ++i) {
            result.at(i, i) = 1.0f;
        }

        return result;
    }

    static Mat4 translation(const Vec3& offset) {
        Mat4 result = identity();

        result.at(0, 3) = offset.x;
        result.at(1, 3) = offset.y;
        result.at(2, 3) = offset.z;

        return result;
    }

    static Mat4 scaling(const Vec3& factors) {
        Mat4 result = identity();

        result.at(0, 0) = factors.x;
        result.at(1, 1) = factors.y;
        result.at(2, 2) = factors.z;

        return result;
    }

    static Mat4 rotationX(float degrees) {
        float angle = degrees * 3.14159265358979323846f / 180.0f;

        float c = std::cos(angle);
        float s = std::sin(angle);

        Mat4 result = identity();

        result.at(1, 1) = c;
        result.at(1, 2) = -s;
        result.at(2, 1) = s;
        result.at(2, 2) = c;

        return result;
    }

    static Mat4 rotationY(float degrees) {
        float angle = degrees * 3.14159265358979323846f / 180.0f;

        float c = std::cos(angle);
        float s = std::sin(angle);

        Mat4 result = identity();

        result.at(0, 0) = c;
        result.at(0, 2) = s;
        result.at(2, 0) = -s;
        result.at(2, 2) = c;

        return result;
    }

    static Mat4 rotationZ(float degrees) {
        float angle = degrees * 3.14159265358979323846f / 180.0f;

        float c = std::cos(angle);
        float s = std::sin(angle);

        Mat4 result = identity();

        result.at(0, 0) = c;
        result.at(0, 1) = -s;
        result.at(1, 0) = s;
        result.at(1, 1) = c;

        return result;
    }

    Mat4 operator*(const Mat4& other) const {
        Mat4 result;

        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                for (int k = 0; k < 4; ++k) {
                    result.at(row, col) +=
                        at(row, k) * other.at(k, col);
                }
            }
        }

        return result;
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
