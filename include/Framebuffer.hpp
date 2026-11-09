#pragma once

#include "Color.hpp"
#include "Shader.hpp"

#include <vector>
#include <string>

struct TexturedVertex {
    float x, y;
    float z;
    float depth;
    float u, v;
};

class Framebuffer {
private:
    int width, height;

    std::vector<Color> pixels;
    std::vector<float> depthBuffer;

public:
    Framebuffer(int width, int height);

    void clear(const Color& color);

    void setPixel(int x, int y, const Color& color);

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

    int getWidth() const;
    int getHeight() const;

    const Color* getPixels() const {
        return pixels.data();
    }
};
