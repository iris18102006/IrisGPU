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

    const ClipVertex* prev = &in.v[in.count - 1];
    float prevDist = planeDistance(plane, prev->clip);

    for (int i = 0; i < in.count; ++i) {
        const ClipVertex& cur = in.v[i];
        const float curDist = planeDistance(plane, cur.clip);

        if ((curDist >= 0.0f) != (prevDist >= 0.0f)) {
            const float t = prevDist / (prevDist - curDist);
            if (out.count < kMaxPolygon) out.v[out.count++] = lerpVertex(*prev, cur, t);
        }

        if (curDist >= 0.0f && out.count < kMaxPolygon) out.v[out.count++] = cur;

        prev = &cur;
        prevDist = curDist;
    }
}

VertexOutput defaultVertexShader(const Vertex& in, const ShaderUniforms& u) {
    VertexOutput out;
    out.clipPosition = u.mvp * Vec4(in.position, 1.0f);
    out.uv = in.uv;
    out.normal = normalize(u.modelView.transformDirection(in.normal));
    return out;
}

}  // namespace

Renderer::Renderer(Framebuffer& target, unsigned threadCount)
    : framebuffer(target),
      threads(threadCount != 0 ? threadCount : std::max(1u, std::thread::hardware_concurrency())) {}

void Renderer::beginFrame(const Color& clearColor) {
    framebuffer.clear(clearColor);
    drawCalls.clear();
    triangles.clear();
    currentStats = {};
}

void Renderer::draw(
    const MeshInstance& instance,
    const Camera& camera,
    const ShaderUniforms& baseUniforms,
    const FragmentShader& fragmentShader,
    const VertexShader* vertexShader
) {
    if (!instance.mesh || !fragmentShader) return;

    const float aspect = static_cast<float>(framebuffer.getWidth()) /
                         static_cast<float>(framebuffer.getHeight());

    DrawCall call;
    call.uniforms = baseUniforms;
    call.fragmentShader = &fragmentShader;

    ShaderUniforms& u = call.uniforms;
    u.model = instance.modelMatrix();
    u.view = camera.viewMatrix();
    u.projection = camera.projectionMatrix(aspect);
    u.modelView = u.view * u.model;
    u.mvp = u.projection * u.modelView;
    u.cameraPosition = camera.position;
    u.yaw = camera.yaw;
    u.pitch = camera.pitch;

    if (instance.texture) {
        u.texture = &instance.texture->pixels;
        u.textureWidth = instance.texture->width;
        u.textureHeight = instance.texture->height;
    }

    drawCalls.push_back(std::move(call));
    const std::size_t drawIndex = drawCalls.size() - 1;
    const ShaderUniforms& uniforms = drawCalls[drawIndex].uniforms;

    const Mesh& mesh = *instance.mesh;
    const int width = framebuffer.getWidth();
    const int height = framebuffer.getHeight();

    for (const MeshTriangle& tri : mesh.triangles) {
        ++currentStats.trianglesSubmitted;

        // ---- vertex stage ----
        Polygon poly;
        poly.count = 3;

        for (int i = 0; i < 3; ++i) {
            Vertex in;
            in.position = mesh.vertices[tri.indices[i]];
            in.uv = tri.uvs[i];
            in.normal = tri.normals[i];

