#pragma once

#include "Color.hpp"
#include "Mat4.hpp"
#include "Vec3.hpp"
#include "Vec4.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <vector>

struct Vec2 {
    float u = 0.0f;
    float v = 0.0f;
};

// ---- Vertex stage -------------------------------------------------------

struct Vertex {
    Vec3 position;   // model space
    Vec2 uv;
    Vec3 normal;     // model space
};

struct VertexOutput {
    Vec4 clipPosition;  // must be in clip space (before the perspective divide)
    Vec2 uv;
    Vec3 normal;        // view space
};

// ---- Fragment stage -----------------------------------------------------

struct FragmentInput {
    Vec2 uv;
    float depth = 0.0f;  // view-space distance along -Z
    Vec3 normal;         // view space, interpolated (NOT normalized)
};

enum class ShadeMode : int {
    TexturedLit = 0,
    TexturedFlat = 1,
    Normals = 2,
    Depth = 3
};

struct ShaderUniforms {
    // Matrices (filled in by the Renderer for every draw call).
    Mat4 model = Mat4::identity();
    Mat4 view = Mat4::identity();
    Mat4 projection = Mat4::identity();
    Mat4 modelView = Mat4::identity();
    Mat4 mvp = Mat4::identity();

    Vec3 cameraPosition;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float time = 0.0f;

    // Lighting (view space; lightDirection points TOWARD the light).
    Vec3 lightDirection = Vec3(0.4f, 0.8f, 0.5f);
    float ambient = 0.18f;
    float specularStrength = 0.35f;
    float shininess = 32.0f;

    ShadeMode mode = ShadeMode::TexturedLit;
    bool bilinear = true;

    const std::vector<Color>* texture = nullptr;
    int textureWidth = 0;
    int textureHeight = 0;
};

using VertexShader = std::function<VertexOutput(const Vertex&, const ShaderUniforms&)>;
using FragmentShader = std::function<Color(const FragmentInput&, const ShaderUniforms&)>;

// ---- Texture sampling (repeat wrap) ------------------------------------

inline float wrap01(float t) { return t - std::floor(t); }

inline Color sampleNearest(const std::vector<Color>& tex, int w, int h, float u, float v) {
    const int tx = std::min(w - 1, static_cast<int>(wrap01(u) * static_cast<float>(w)));
    const int ty = std::min(h - 1, static_cast<int>(wrap01(v) * static_cast<float>(h)));
    return tex[static_cast<std::size_t>(ty) * w + tx];
}

inline Color sampleBilinear(const std::vector<Color>& tex, int w, int h, float u, float v) {
    const float fx = wrap01(u) * static_cast<float>(w) - 0.5f;
    const float fy = wrap01(v) * static_cast<float>(h) - 0.5f;

    const int x0 = static_cast<int>(std::floor(fx));
    const int y0 = static_cast<int>(std::floor(fy));
    const float tx = fx - static_cast<float>(x0);
    const float ty = fy - static_cast<float>(y0);

    auto texel = [&](int x, int y) -> const Color& {
        x = ((x % w) + w) % w;
        y = ((y % h) + h) % h;
        return tex[static_cast<std::size_t>(y) * w + x];
    };

    const Color& c00 = texel(x0, y0);
    const Color& c10 = texel(x0 + 1, y0);
    const Color& c01 = texel(x0, y0 + 1);
    const Color& c11 = texel(x0 + 1, y0 + 1);

    auto mix = [&](std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d) {
        const float top = static_cast<float>(a) * (1 - tx) + static_cast<float>(b) * tx;
        const float bot = static_cast<float>(c) * (1 - tx) + static_cast<float>(d) * tx;
        return static_cast<std::uint8_t>(std::clamp(top * (1 - ty) + bot * ty, 0.0f, 255.0f) + 0.5f);
    };

