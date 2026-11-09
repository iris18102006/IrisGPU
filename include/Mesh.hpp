
#pragma once

#include "Vec3.hpp"
#include "Shader.hpp"

#include <array>
#include <string>
#include <vector>

struct MeshTriangle {
    std::array<int, 3> indices;
    std::array<Vec2, 3> uvs;
};

struct Mesh {
    std::string name;

    std::vector<Vec3> vertices;
    std::vector<MeshTriangle> triangles;
};

struct MeshInstance {
    const Mesh* mesh = nullptr;

    Vec3 position;
    Vec3 rotation;
    Vec3 scale = Vec3(1, 1, 1);
};
