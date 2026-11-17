#include "Renderer.hpp"

#include "Viewport.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <thread>

namespace {

struct ClipVertex {
    Vec4 clip;
    Vec2 uv;
    Vec3 normal;
};

ClipVertex lerpVertex(const ClipVertex& a, const ClipVertex& b, float t) {
    return {
        lerp(a.clip, b.clip, t),
        {a.uv.u + (b.uv.u - a.uv.u) * t, a.uv.v + (b.uv.v - a.uv.v) * t},
        lerp(a.normal, b.normal, t)
    };
}

// Signed distance to each of the 6 frustum planes; >= 0 means inside.
float planeDistance(int plane, const Vec4& p) {
    switch (plane) {
        case 0: return p.w + p.x;
        case 1: return p.w - p.x;
        case 2: return p.w + p.y;
        case 3: return p.w - p.y;
        case 4: return p.w + p.z;  // near
        default: return p.w - p.z; // far
    }
}

constexpr int kMaxPolygon = 12;  // 3 vertices + at most 1 per clip plane (+ slack)

struct Polygon {
    std::array<ClipVertex, kMaxPolygon> v;
    int count = 0;
};

void clipAgainstPlane(const Polygon& in, Polygon& out, int plane) {
    out.count = 0;
    if (in.count == 0) return;

