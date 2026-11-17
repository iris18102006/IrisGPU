#include "Camera.hpp"
#include "Framebuffer.hpp"
#include "Mat4.hpp"
#include "MeshFactory.hpp"
#include "ObjLoader.hpp"
#include "Renderer.hpp"
#include "Vec3.hpp"

#include <cmath>
#include <cstring>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

bool near(float a, float b, float eps = 1e-4f) { return std::abs(a - b) <= eps; }
bool near(const Vec3& a, const Vec3& b, float eps = 1e-4f) {
    return near(a.x, b.x, eps) && near(a.y, b.y, eps) && near(a.z, b.z, eps);
}

const Color kBg(10, 10, 10);
const Color kRed(255, 0, 0);

FragmentShader solidRed() {
    return [](const FragmentInput&, const ShaderUniforms&) { return kRed; };
}

// ---------------- Vec3 / Mat4 ----------------

bool testVec3Basics() {
    const Vec3 a(1, 0, 0), b(0, 1, 0);
    return near(cross(a, b), Vec3(0, 0, 1)) &&
           near(dot(a, b), 0.0f) &&
           near(length(normalize(Vec3(3, 4, 0))), 1.0f) &&
           near(Vec3(1, 2, 3) + Vec3(1, 1, 1), Vec3(2, 3, 4)) &&
           near(normalize(Vec3(0, 0, 0)), Vec3(0, 0, 0));
}

bool testMat4Translation() {
    const Mat4 m = Mat4::translation(Vec3(1, 2, 3));
    return near(m.transformPoint(Vec3(1, 1, 1)), Vec3(2, 3, 4)) &&
           near(m.transformDirection(Vec3(1, 1, 1)), Vec3(1, 1, 1));
}

bool testMat4RotationY() {
    // +90 degrees around Y sends +X to -Z.
    return near(Mat4::rotationY(90).transformPoint(Vec3(1, 0, 0)), Vec3(0, 0, -1));
}

bool testMat4Multiply() {
    const Mat4 m = Mat4::translation(Vec3(5, 0, 0)) * Mat4::scaling(Vec3(2, 2, 2));
    return near(m.transformPoint(Vec3(1, 1, 1)), Vec3(7, 2, 2));
}

bool testLookAt() {
    const Vec3 eye(3, 2, 5);
    const Mat4 view = Mat4::lookAt(eye, eye + Vec3(0, 0, -1), Vec3(0, 1, 0));

    return near(view.transformPoint(eye), Vec3(0, 0, 0)) &&
           near(view.transformPoint(eye + Vec3(0, 0, -3)), Vec3(0, 0, -3)) &&
           near(view.transformPoint(eye + Vec3(2, 0, 0)), Vec3(2, 0, 0));
}

bool testPerspectiveMapsNearFar() {
    const Mat4 p = Mat4::perspective(90.0f, 1.0f, 0.1f, 100.0f);

    const Vec4 nearP = p * Vec4(0, 0, -0.1f, 1);
    const Vec4 farP = p * Vec4(0, 0, -100.0f, 1);

    return near(nearP.z / nearP.w, -1.0f) &&
           near(farP.z / farP.w, 1.0f, 1e-3f) &&
           near(nearP.w, 0.1f) &&
           near(farP.w, 100.0f);
}

bool testPerspectiveFov() {
    // With a 90 degree FOV, a point at x == -z lands on the right edge of NDC.
    const Mat4 p = Mat4::perspective(90.0f, 1.0f, 0.1f, 100.0f);
    const Vec4 c = p * Vec4(2, 0, -2, 1);
    return near(c.x / c.w, 1.0f);
}

bool testCameraForward() {
    Camera c;
    return near(c.forward(), Vec3(0, 0, -1)) && near(c.right(), Vec3(1, 0, 0));
}

// ---------------- Meshes ----------------

// Every triangle must be wound CCW seen from outside, i.e. its geometric normal
// agrees with the stored (outward) per-corner normals.
bool windingMatchesNormals(const Mesh& m) {
    int checked = 0;

    for (const MeshTriangle& t : m.triangles) {
        const Vec3& a = m.vertices[t.indices[0]];
        const Vec3& b = m.vertices[t.indices[1]];
        const Vec3& c = m.vertices[t.indices[2]];

        const Vec3 geometric = cross(b - a, c - a);
        if (length(geometric) < 1e-6f) continue;  // pole slivers

        const Vec3 stored = t.normals[0] + t.normals[1] + t.normals[2];
        if (dot(geometric, stored) <= 0.0f) return false;
        ++checked;
    }

    return checked > 0;
}

bool testCubeMesh() {
    const Mesh cube = createCubeMesh();
    if (cube.triangles.size() != 12) return false;

    for (const MeshTriangle& t : cube.triangles) {
        const Vec3 centroid = (cube.vertices[t.indices[0]] +
                               cube.vertices[t.indices[1]] +
                               cube.vertices[t.indices[2]]) / 3.0f;
        if (dot(t.normals[0], centroid) <= 0.0f) return false;  // must point away from the center
    }

    return windingMatchesNormals(cube);
}

bool testSphereMesh() {
    const Mesh s = createSphereMesh(12, 24);
    for (const Vec3& v : s.vertices) {
        if (!near(length(v), 1.0f)) return false;
    }
    return windingMatchesNormals(s);
}

bool testTorusMesh() { return windingMatchesNormals(createTorusMesh(1.0f, 0.35f, 24, 12)); }
bool testPlaneMesh() {
    const Mesh p = createPlaneMesh();
    return windingMatchesNormals(p) && near(p.triangles[0].normals[0], Vec3(0, 1, 0));
}

bool testNormalizeToUnitSize() {
    Mesh m = createCubeMesh();
    for (Vec3& v : m.vertices) v = v * 10.0f + Vec3(100, 0, 0);
    m.normalizeToUnitSize();

    float maxLen = 0.0f;
    Vec3 sum;
    for (const Vec3& v : m.vertices) {
        maxLen = std::max(maxLen, length(v));
        sum += v;
    }
    return near(maxLen, 1.0f) && near(sum / static_cast<float>(m.vertices.size()), Vec3(0, 0, 0));
}

// ---------------- OBJ ----------------

bool testObjQuadAndNegativeIndices() {
    std::istringstream in(
        "# a quad\n"
        "v 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\n"
        "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\n"
        "f 1/1 2/2 3/3 4/4\n"
        "f -4 -3 -2\n"
    );

    Mesh m;
    std::string err;
    if (!parseObj(in, m, &err)) return false;

    // quad -> 2 triangles, plus 1 triangle from the negative-index face.
    return m.triangles.size() == 3 &&
           m.vertices.size() == 4 &&
           m.triangles[0].indices == std::array<int, 3>{0, 1, 2} &&
           m.triangles[1].indices == std::array<int, 3>{0, 2, 3} &&
           m.triangles[2].indices == std::array<int, 3>{0, 1, 2} &&
           near(m.triangles[0].uvs[2].u, 1.0f);
}

bool testObjGeneratesNormals() {
    std::istringstream in("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n");
    Mesh m;
    return parseObj(in, m) && near(m.triangles[0].normals[0], Vec3(0, 0, 1));
}

bool testObjUsesGivenNormals() {
    std::istringstream in("v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 0 0 2\nf 1//1 2//1 3//1\n");
    Mesh m;
    return parseObj(in, m) && near(m.triangles[0].normals[1], Vec3(0, 0, 1));
}

bool testObjRejectsBadInput() {
    Mesh m;
    std::string err;

    std::istringstream badIndex("v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 9\n");
    std::istringstream noFaces("v 0 0 0\n");
    std::istringstream badVertex("v 0 zero 0\n");

    return !parseObj(badIndex, m, &err) && !err.empty() &&
           !parseObj(noFaces, m) &&
           !parseObj(badVertex, m);
}

// ---------------- Renderer ----------------

Camera frontCamera() {
    Camera c;
    c.position = Vec3(0, 0, 3);
    return c;
}

Mesh singleTriangle(Vec3 a, Vec3 b, Vec3 c) {
    Mesh m;
    m.vertices = {a, b, c};
    MeshTriangle t;
    t.indices = {0, 1, 2};
    t.normals = {Vec3(0, 0, 1), Vec3(0, 0, 1), Vec3(0, 0, 1)};
    m.triangles.push_back(t);
    return m;
}

std::size_t countColor(const Framebuffer& fb, const Color& color) {
    std::size_t n = 0;
    for (int i = 0; i < fb.getWidth() * fb.getHeight(); ++i) {
        if (fb.getPixels()[i] == color) ++n;
    }
    return n;
}

bool testRendererDrawsFrontFace() {
    Framebuffer fb(64, 64);
    Renderer r(fb, 1);

    const Mesh tri = singleTriangle(Vec3(-1, -1, 0), Vec3(1, -1, 0), Vec3(0, 1, 0));  // CCW facing +Z
    MeshInstance inst;
    inst.mesh = &tri;

    const FragmentShader shader = solidRed();
    r.beginFrame(kBg);
    r.draw(inst, frontCamera(), ShaderUniforms{}, shader);
    r.endFrame();

    return *fb.getPixel(32, 36) == kRed && *fb.getPixel(1, 1) == kBg;
}

bool testRendererCullsBackFace() {
    Framebuffer fb(64, 64);
    Renderer r(fb, 1);

    // Same triangle, wound the other way: facing away from the camera.
    const Mesh tri = singleTriangle(Vec3(-1, -1, 0), Vec3(0, 1, 0), Vec3(1, -1, 0));
    MeshInstance inst;
    inst.mesh = &tri;

    const FragmentShader shader = solidRed();

    r.beginFrame(kBg);
    r.draw(inst, frontCamera(), ShaderUniforms{}, shader);
    r.endFrame();
    const bool culled = countColor(fb, kRed) == 0;

    r.setBackfaceCulling(false);
    r.beginFrame(kBg);
    r.draw(inst, frontCamera(), ShaderUniforms{}, shader);
    r.endFrame();
    const bool drawnWhenDisabled = countColor(fb, kRed) > 0;

    return culled && drawnWhenDisabled;
}

