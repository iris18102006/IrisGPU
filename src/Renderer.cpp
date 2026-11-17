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

            const VertexOutput out = vertexShader ? (*vertexShader)(in, uniforms)
                                                  : defaultVertexShader(in, uniforms);
            poly.v[i] = {out.clipPosition, out.uv, out.normal};
        }

        // ---- trivial reject / accept against the frustum ----
        bool rejected = false;
        bool needsClip = false;

        for (int plane = 0; plane < 6 && !rejected; ++plane) {
            int outside = 0;
            for (int i = 0; i < 3; ++i) {
                if (planeDistance(plane, poly.v[i].clip) < 0.0f) ++outside;
            }
            if (outside == 3) rejected = true;
            else if (outside > 0) needsClip = true;
        }

        if (rejected) continue;

        if (needsClip) {
            Polygon scratch;
            Polygon* src = &poly;
            Polygon* dst = &scratch;

            for (int plane = 0; plane < 6; ++plane) {
                clipAgainstPlane(*src, *dst, plane);
                std::swap(src, dst);
                if (src->count < 3) break;
            }

            if (src->count < 3) continue;
            if (src != &poly) poly = *src;
        }

        // ---- perspective divide + viewport ----
        std::array<TexturedVertex, kMaxPolygon> projected;
        std::array<Vec3, kMaxPolygon> ndc;

        for (int i = 0; i < poly.count; ++i) {
            const ClipVertex& cv = poly.v[i];
            if (cv.clip.w <= 1e-6f) { poly.count = 0; break; }

            const float invW = 1.0f / cv.clip.w;
            ndc[i] = Vec3(cv.clip.x * invW, cv.clip.y * invW, cv.clip.z * invW);

            const Vec3 screen = Viewport::toScreen(ndc[i], width, height);

            TexturedVertex tv;
            tv.x = screen.x;
            tv.y = screen.y;
            tv.z = screen.z;
            tv.depth = cv.clip.w;
            tv.u = cv.uv.u;
            tv.v = cv.uv.v;
            tv.normal = cv.normal;
            projected[i] = tv;
        }

        if (poly.count < 3) continue;

        // ---- backface culling: CCW in NDC (y up) is front-facing ----
        if (cullBackfaces) {
            float signedArea = 0.0f;
            for (int i = 0; i < poly.count; ++i) {
                const Vec3& p = ndc[i];
                const Vec3& q = ndc[(i + 1) % poly.count];
                signedArea += p.x * q.y - q.x * p.y;
            }
            if (signedArea <= 0.0f) continue;
        }

        // ---- fan triangulation ----
        for (int i = 1; i + 1 < poly.count; ++i) {
            ScreenTriangle st;
            st.v[0] = projected[0];
            st.v[1] = projected[i];
            st.v[2] = projected[i + 1];
            st.drawIndex = drawIndex;

            const float lo = std::min({st.v[0].y, st.v[1].y, st.v[2].y});
            const float hi = std::max({st.v[0].y, st.v[1].y, st.v[2].y});
            st.minY = std::max(0, static_cast<int>(std::floor(lo)));
            st.maxY = std::min(height - 1, static_cast<int>(std::ceil(hi)));

            if (st.minY > st.maxY) continue;

            triangles.push_back(st);
            ++currentStats.trianglesRasterized;
        }
    }
}

