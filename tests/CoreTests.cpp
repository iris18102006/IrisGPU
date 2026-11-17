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

