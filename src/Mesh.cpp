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

        const Vec3 n = normalize(cross(b - a, c - a));
        tri.normals = {n, n, n};
    }
}

void Mesh::computeSmoothNormals() {
    std::vector<Vec3> accum(vertices.size());

    for (const MeshTriangle& tri : triangles) {
        const Vec3& a = vertices[tri.indices[0]];
        const Vec3& b = vertices[tri.indices[1]];
        const Vec3& c = vertices[tri.indices[2]];

        const Vec3 weighted = cross(b - a, c - a);  // length ~ 2 * area
        for (int i = 0; i < 3; ++i) accum[tri.indices[i]] += weighted;
    }

    for (Vec3& n : accum) n = normalize(n);

    for (MeshTriangle& tri : triangles) {
        for (int i = 0; i < 3; ++i) tri.normals[i] = accum[tri.indices[i]];
    }
}

