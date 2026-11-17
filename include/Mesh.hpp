#pragma once

#include "Mat4.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "Vec3.hpp"

#include <array>
#include <string>
#include <vector>

struct MeshTriangle {
    std::array<int, 3> indices{};   // positions, wound counter-clockwise when seen from outside
    std::array<Vec2, 3> uvs{};
    std::array<Vec3, 3> normals{};  // per-corner, model space
};

struct Mesh {
    std::string name;

    std::vector<Vec3> vertices;
    std::vector<MeshTriangle> triangles;

    // One normal per triangle, copied to all three corners (faceted look).
    void computeFlatNormals();

    // Area-weighted average of the faces touching each vertex (smooth look).
    void computeSmoothNormals();

};

struct MeshInstance {
    const Mesh* mesh = nullptr;

    Vec3 position;
    Vec3 rotation;
    Vec3 scale = Vec3(1, 1, 1);
};
