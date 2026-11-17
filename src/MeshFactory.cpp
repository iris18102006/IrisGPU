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

    auto idx = [&](int i, int j) { return i * slices + (j % slices); };
    auto uv = [&](int i, int j) {
        return Vec2{static_cast<float>(j) / static_cast<float>(slices),
                    static_cast<float>(i) / static_cast<float>(stacks)};
    };

    auto addTri = [&](int i0, int j0, int i1, int j1, int i2, int j2) {
        MeshTriangle t;
        t.indices = {idx(i0, j0), idx(i1, j1), idx(i2, j2)};
        t.uvs = {uv(i0, j0), uv(i1, j1), uv(i2, j2)};
        for (int k = 0; k < 3; ++k) t.normals[k] = normalize(m.vertices[t.indices[k]]);
        m.triangles.push_back(t);
    };

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            // Poles collapse one of the two triangles to a sliver; skip it.
            if (i != stacks - 1) addTri(i, j, i + 1, j + 1, i + 1, j);
            if (i != 0) addTri(i, j, i, j + 1, i + 1, j + 1);
        }
    }

    return m;
}

Mesh createPlaneMesh(float halfSize, float uvTiles) {
    Mesh m;
    m.name = "Plane";

    const Vec3 t(0, 0, 1);
    const Vec3 b(1, 0, 0);  // cross(t, b) = +Y
    auto corner = [&](float sx, float sy) { return t * (sx * halfSize) + b * (sy * halfSize); };

    addQuad(m, corner(-1, -1), corner(1, -1), corner(1, 1), corner(-1, 1),
            {0, uvTiles}, {uvTiles, uvTiles}, {uvTiles, 0}, {0, 0});

    m.computeFlatNormals();
    return m;
}

Mesh createTorusMesh(float majorRadius, float minorRadius, int segments, int sides) {
    Mesh m;
    m.name = "Torus";

    for (int i = 0; i < segments; ++i) {
        const float u = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(segments);
        for (int j = 0; j < sides; ++j) {
            const float v = 2.0f * kPi * static_cast<float>(j) / static_cast<float>(sides);
            const float ring = majorRadius + minorRadius * std::cos(v);
            m.vertices.push_back({ring * std::cos(u), minorRadius * std::sin(v), ring * std::sin(u)});
        }
    }

