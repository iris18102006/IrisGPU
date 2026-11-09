#pragma once

#include "Vec3.hpp"
#include <cmath>
#include <stdexcept>

class Projection {
public:
    static Vec3 perspective(
        const Vec3& point,
        float fovDegrees,
        float aspectRatio,
        float nearPlane,
        float farPlane
    ) {
        if (point.z >= -nearPlane || point.z <= -farPlane) {
            throw std::out_of_range("Point outside clipping planes");
        }

        float fovRadians =
            fovDegrees * 3.14159265358979323846f / 180.0f;

        float focalLength = 1.0f / std::tan(fovRadians / 2.0f);

        float x = point.x * focalLength / aspectRatio;
        float y = point.y * focalLength;
        float z =
            (farPlane + nearPlane) / (nearPlane - farPlane) * point.z
            + (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);

        float w = -point.z;

        return Vec3(x / w, y / w, z / w);
    }
};
