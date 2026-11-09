#pragma once

#include "Vec3.hpp"

class Viewport {
public:
    static Vec3 toScreen(
        const Vec3& point,
        int width,
        int height
    ) {
        return Vec3(
            (point.x + 1.0f) * 0.5f * width,
            (1.0f - point.y) * 0.5f * height,
            point.z
        );
    }
};
