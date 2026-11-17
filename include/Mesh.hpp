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

    // Re-centers on the origin and scales so the farthest vertex is at distance 1.
    void normalizeToUnitSize();
};

struct MeshInstance {
    const Mesh* mesh = nullptr;
    const Texture* texture = nullptr;

    Vec3 position;
    Vec3 rotation;  // degrees around X, Y, Z
    Vec3 scale = Vec3(1, 1, 1);

    Mat4 modelMatrix() const {
        return Mat4::translation(position) *
               Mat4::rotationY(rotation.y) *
               Mat4::rotationX(rotation.x) *
               Mat4::rotationZ(rotation.z) *
               Mat4::scaling(scale);
    }
};
