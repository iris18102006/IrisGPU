#include "Mesh.hpp"

#include <algorithm>

Texture createCheckerTexture(int width, int height, int cellSize, Color a, Color b) {
    Texture t;
    t.width = width;
    t.height = height;
    t.pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const bool alt = ((x / cellSize) + (y / cellSize)) % 2 == 0;
            t.pixels[static_cast<std::size_t>(y) * width + x] = alt ? a : b;
        }
    }

    return t;
}

void Mesh::computeFlatNormals() {
    for (MeshTriangle& tri : triangles) {
        const Vec3& a = vertices[tri.indices[0]];
        const Vec3& b = vertices[tri.indices[1]];
        const Vec3& c = vertices[tri.indices[2]];

