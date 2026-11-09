#pragma once

#include "Vec3.hpp"
#include <cmath>

namespace Transform {

    constexpr float PI = 3.14159265358979323846f;

    inline float toRadians(float degrees) {
        return degrees * PI / 180.0f;
    }

    inline Vec3 translate(
        const Vec3& vertex,
        const Vec3& offset
    ) {
        return Vec3(
            vertex.x + offset.x,
            vertex.y + offset.y,
            vertex.z + offset.z
        );
    }

    inline Vec3 scale(
        const Vec3& vertex,
        const Vec3& factors
    ) {
        return Vec3(
            vertex.x * factors.x,
            vertex.y * factors.y,
            vertex.z * factors.z
        );
    }

    inline Vec3 rotateX(const Vec3& vertex, float degrees) {
        float angle = toRadians(degrees);

        float c = std::cos(angle);
        float s = std::sin(angle);

        return Vec3(
            vertex.x,
            vertex.y * c - vertex.z * s,
            vertex.y * s + vertex.z * c
        );
    }

    inline Vec3 rotateY(const Vec3& vertex, float degrees) {
        float angle = toRadians(degrees);

        float c = std::cos(angle);
        float s = std::sin(angle);

        return Vec3(
            vertex.x * c + vertex.z * s,
            vertex.y,
            -vertex.x * s + vertex.z * c
        );
    }

    inline Vec3 rotateZ(const Vec3& vertex, float degrees) {
        float angle = toRadians(degrees);

        float c = std::cos(angle);
        float s = std::sin(angle);

        return Vec3(
            vertex.x * c - vertex.y * s,
            vertex.x * s + vertex.y * c,
            vertex.z
        );
    }

}
