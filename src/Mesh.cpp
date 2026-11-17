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

void Mesh::normalizeToUnitSize() {
    if (vertices.empty()) return;

    Vec3 lo = vertices[0];
    Vec3 hi = vertices[0];

    for (const Vec3& v : vertices) {
        lo = {std::min(lo.x, v.x), std::min(lo.y, v.y), std::min(lo.z, v.z)};
        hi = {std::max(hi.x, v.x), std::max(hi.y, v.y), std::max(hi.z, v.z)};
    }

    const Vec3 center = (lo + hi) * 0.5f;
    float radius = 0.0f;
    for (const Vec3& v : vertices) radius = std::max(radius, length(v - center));
    if (radius < 1e-6f) return;

    for (Vec3& v : vertices) v = (v - center) / radius;
}
