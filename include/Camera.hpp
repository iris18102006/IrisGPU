#pragma once

#include "Mat4.hpp"
#include "Vec3.hpp"

#include <cmath>

struct Camera {
    Vec3 position;
    float yaw = 0.0f;    // radians, 0 looks down -Z, positive turns right
    float pitch = 0.0f;  // radians, positive looks up

    float fovDegrees = 70.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    Vec3 forward() const {
        return normalize(Vec3(
            std::sin(yaw) * std::cos(pitch),
            std::sin(pitch),
            -std::cos(yaw) * std::cos(pitch)
        ));
    }

