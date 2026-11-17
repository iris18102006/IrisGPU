#pragma once

#include "Color.hpp"

#include <vector>

struct Texture {
    int width = 0;
    int height = 0;
    std::vector<Color> pixels;
};

Texture createCheckerTexture(int width, int height, int cellSize, Color a, Color b);
