#pragma once

#include "Vec3.hpp"

class Viewport {
public:
    // NDC ([-1,1], y up) to pixel coordinates (y down). z is passed through.
    static Vec3 toScreen(const Vec3& ndc, int width, int height) {
        return Vec3(
            (ndc.x + 1.0f) * 0.5f * static_cast<float>(width),
            (1.0f - ndc.y) * 0.5f * static_cast<float>(height),
            ndc.z
        );
    }
};
