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

    // Horizontal right vector (ignores pitch so strafing stays on the ground plane).
    Vec3 right() const { return normalize(Vec3(std::cos(yaw), 0.0f, std::sin(yaw))); }

    Mat4 viewMatrix() const {
        return Mat4::lookAt(position, position + forward(), Vec3(0, 1, 0));
    }

    Mat4 projectionMatrix(float aspect) const {
        return Mat4::perspective(fovDegrees, aspect, nearPlane, farPlane);
    }
};
