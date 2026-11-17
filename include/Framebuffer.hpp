#pragma once

#include "Color.hpp"
#include "Shader.hpp"

#include <cstddef>
#include <string>
#include <vector>

struct TexturedVertex {
    float x = 0.0f, y = 0.0f;   // screen space (pixels)
    float z = 0.0f;             // NDC depth, linear in screen space
    float depth = 0.0f;         // view-space depth (clip w), used for perspective correction
    float u = 0.0f, v = 0.0f;
    Vec3 normal;
};

class Framebuffer {
private:
    int width;
    int height;

    std::vector<Color> pixels;
    std::vector<float> depthBuffer;

    std::size_t getIndex(int x, int y) const;

public:
    Framebuffer(int width, int height);

    void clear(const Color& color);
    void setPixel(int x, int y, const Color& color);

    const Color* getPixels() const;
    const Color* getPixel(int x, int y) const;

    int getWidth() const;
    int getHeight() const;

    void drawTriangle(
        int x0, int y0,
        int x1, int y1,
        int x2, int y2,
        const Color& color
    );

    void drawTriangle3D(
        int x0, int y0, float z0,
        int x1, int y1, float z1,
        int x2, int y2, float z2,
        const Color& color
    );

    // Existing texture-rendering interface.
    void drawTexturedTriangle3D(
        const TexturedVertex& a,
        const TexturedVertex& b,
        const TexturedVertex& c,
        const std::vector<Color>& texture,
        int textureWidth,
        int textureHeight
    );

    // Programmable fragment-shader interface.
    void drawTexturedTriangle3D(
        const TexturedVertex& a,
        const TexturedVertex& b,
        const TexturedVertex& c,
        const FragmentShader& fragmentShader,
        const ShaderUniforms& uniforms
    );

    bool savePPM(const std::string& filename) const;
};
