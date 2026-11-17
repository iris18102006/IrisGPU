#include "MeshFactory.hpp"

#include <cmath>

namespace {

constexpr float kPi = 3.14159265358979323846f;

// Adds one quad (2 triangles, CCW seen from outside) to the mesh.
void addQuad(Mesh& m, const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3,
             Vec2 uv0, Vec2 uv1, Vec2 uv2, Vec2 uv3) {
    const int base = static_cast<int>(m.vertices.size());
    m.vertices.push_back(p0);
    m.vertices.push_back(p1);
    m.vertices.push_back(p2);
    m.vertices.push_back(p3);

    MeshTriangle t0;
    t0.indices = {base, base + 1, base + 2};
    t0.uvs = {uv0, uv1, uv2};

    MeshTriangle t1;
    t1.indices = {base, base + 2, base + 3};
    t1.uvs = {uv0, uv2, uv3};

    m.triangles.push_back(t0);
    m.triangles.push_back(t1);
}

}  // namespace

Mesh createCubeMesh() {
    Mesh cube;
    cube.name = "Cube";

    struct Face { Vec3 n, t, b; };
    // Each face satisfies cross(t, b) == n, so (-1,-1) -> (1,-1) -> (1,1) -> (-1,1) is CCW from outside.
    const Face faces[6] = {
        {{ 1, 0, 0}, {0, 1, 0}, {0, 0, 1}},
        {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}},
        {{ 0, 1, 0}, {0, 0, 1}, {1, 0, 0}},
        {{ 0,-1, 0}, {1, 0, 0}, {0, 0, 1}},
        {{ 0, 0, 1}, {1, 0, 0}, {0, 1, 0}},
        {{ 0, 0,-1}, {0, 1, 0}, {1, 0, 0}},
    };

    for (const Face& f : faces) {
        auto corner = [&](float sx, float sy) { return f.n + f.t * sx + f.b * sy; };
        addQuad(cube,
                corner(-1, -1), corner(1, -1), corner(1, 1), corner(-1, 1),
                {0, 1}, {1, 1}, {1, 0}, {0, 0});
    }

    cube.computeFlatNormals();
    return cube;
}

Mesh createSphereMesh(int stacks, int slices) {
    Mesh m;
    m.name = "Sphere";

    for (int i = 0; i <= stacks; ++i) {
        const float theta = kPi * static_cast<float>(i) / static_cast<float>(stacks);
        for (int j = 0; j < slices; ++j) {
            const float phi = 2.0f * kPi * static_cast<float>(j) / static_cast<float>(slices);
            m.vertices.push_back({
                std::sin(theta) * std::cos(phi),
                std::cos(theta),
                std::sin(theta) * std::sin(phi)
            });
        }
    }

