#pragma once

#include "Color.hpp"
#include "Vec3.hpp"
#include "Mat4.hpp"

#include <functional>
#include <vector>

struct Vec2 {
    float u, v;
};

struct Vertex {
    Vec3 position;
    Vec2 uv;
};

struct VertexOutput {
    Vec3 position;
    Vec2 uv;
};

struct FragmentInput {
    Vec2 uv;
    float depth;
};

struct ShaderUniforms {
    Mat4 model;
    Vec3 cameraPosition;

    float yaw = 0.0f;
    float pitch = 0.0f;
    float time = 0.0f;

    const std::vector<Color>* texture = nullptr;

    int textureWidth = 0;
    int textureHeight = 0;
};

using VertexShader = std::function<VertexOutput(
    const Vertex&,
    const ShaderUniforms&
)>;

using FragmentShader = std::function<Color(
    const FragmentInput&,
    const ShaderUniforms&
)>;
