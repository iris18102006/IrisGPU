#pragma once

#include <cstdint>

struct Color {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;

    constexpr Color() : r(0), g(0), b(0) {}

    constexpr Color(
        std::uint8_t red,
        std::uint8_t green,
        std::uint8_t blue
    ) : r(red), g(green), b(blue) {}

    constexpr bool operator==(const Color& other) const {
        return r == other.r &&
               g == other.g &&
               b == other.b;
    }

    constexpr bool operator!=(const Color& other) const {
        return !(*this == other);
    }
};
