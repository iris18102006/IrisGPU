#include "Framebuffer.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace {

std::size_t checkedPixelCount(int width, int height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Framebuffer dimensions must be positive");
    }

    const auto w = static_cast<std::size_t>(width);
    const auto h = static_cast<std::size_t>(height);

    if (w > std::numeric_limits<std::size_t>::max() / h) {
        throw std::length_error("Framebuffer dimensions overflow");
    }

    return w * h;
}

constexpr float kEdgeEpsilon = -1e-4f;  // tiny tolerance so shared edges leave no cracks

}  // namespace

Framebuffer::Framebuffer(int width, int height)
    : width(width),
      height(height),
      pixels(checkedPixelCount(width, height)),
      depthBuffer(pixels.size(), std::numeric_limits<float>::infinity()) {}

void Framebuffer::clear(const Color& color) {
    std::fill(pixels.begin(), pixels.end(), color);
    std::fill(depthBuffer.begin(), depthBuffer.end(), std::numeric_limits<float>::infinity());
}

void Framebuffer::setPixel(int x, int y, const Color& color) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    pixels[getIndex(x, y)] = color;
}

void Framebuffer::drawTriangle(
    int x0, int y0, int x1, int y1, int x2, int y2, const Color& color
) {
    const float area = static_cast<float>((x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0));
    if (std::abs(area) < 0.0001f) return;

    const int minX = std::max(0, std::min({x0, x1, x2}));
    const int maxX = std::min(width - 1, std::max({x0, x1, x2}));
    const int minY = std::max(0, std::min({y0, y1, y2}));
    const int maxY = std::min(height - 1, std::max({y0, y1, y2}));

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const float w0 = static_cast<float>((x1 - x) * (y2 - y) - (y1 - y) * (x2 - x)) / area;
            const float w1 = static_cast<float>((x2 - x) * (y0 - y) - (y2 - y) * (x0 - x)) / area;
            const float w2 = 1.0f - w0 - w1;

            if (w0 >= 0 && w1 >= 0 && w2 >= 0) setPixel(x, y, color);
        }
    }
}

void Framebuffer::drawTriangle3D(
    int x0, int y0, float z0,
    int x1, int y1, float z1,
    int x2, int y2, float z2,
    const Color& color
) {
    const float area = static_cast<float>((x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0));
    if (std::abs(area) < 0.0001f) return;

    const int minX = std::max(0, std::min({x0, x1, x2}));
    const int maxX = std::min(width - 1, std::max({x0, x1, x2}));
    const int minY = std::max(0, std::min({y0, y1, y2}));
    const int maxY = std::min(height - 1, std::max({y0, y1, y2}));

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const float w0 = static_cast<float>((x1 - x) * (y2 - y) - (y1 - y) * (x2 - x)) / area;
            const float w1 = static_cast<float>((x2 - x) * (y0 - y) - (y2 - y) * (x0 - x)) / area;
            const float w2 = 1.0f - w0 - w1;

            if (w0 < 0 || w1 < 0 || w2 < 0) continue;

            const float z = w0 * z0 + w1 * z1 + w2 * z2;
            const std::size_t index = getIndex(x, y);

            if (z < depthBuffer[index]) {
                depthBuffer[index] = z;
                pixels[index] = color;
            }
        }
    }
}

void Framebuffer::drawTexturedTriangle3D(
    const TexturedVertex& a,
    const TexturedVertex& b,
    const TexturedVertex& c,
    const std::vector<Color>& texture,
    int textureWidth,
    int textureHeight
) {
    ShaderUniforms uniforms;
    uniforms.texture = &texture;
    uniforms.textureWidth = textureWidth;
    uniforms.textureHeight = textureHeight;

    const FragmentShader shader = [](const FragmentInput& in, const ShaderUniforms& u) {
        return sampleTexture(u, in.uv);
    };

    drawTexturedTriangle3D(a, b, c, shader, uniforms);
}

void Framebuffer::drawTexturedTriangle3D(
    const TexturedVertex& a,
    const TexturedVertex& b,
    const TexturedVertex& c,
    const FragmentShader& fragmentShader,
    const ShaderUniforms& uniforms,
    int yMin,
    int yMax
) {
    if (!fragmentShader || a.depth <= 0 || b.depth <= 0 || c.depth <= 0) return;

    if (yMax < 0 || yMax > height) yMax = height;
    yMin = std::max(0, yMin);

    const float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (std::abs(area) < 0.0001f) return;
    const float invArea = 1.0f / area;

    const int minX = std::max(0, static_cast<int>(std::floor(std::min({a.x, b.x, c.x}))));
    const int maxX = std::min(width - 1, static_cast<int>(std::ceil(std::max({a.x, b.x, c.x}))));
    const int minY = std::max(yMin, static_cast<int>(std::floor(std::min({a.y, b.y, c.y}))));
    const int maxY = std::min(yMax - 1, static_cast<int>(std::ceil(std::max({a.y, b.y, c.y}))));

    if (minX > maxX || minY > maxY) return;

    // Barycentric weights are linear in (x, y), so step them instead of recomputing.
    const float dw0dx = (b.y - c.y) * invArea;
    const float dw1dx = (c.y - a.y) * invArea;

    const float invDa = 1.0f / a.depth;
    const float invDb = 1.0f / b.depth;
    const float invDc = 1.0f / c.depth;

    // Attributes pre-divided by depth for perspective-correct interpolation.
    const float ua = a.u * invDa, ub = b.u * invDb, uc = c.u * invDc;
    const float va = a.v * invDa, vb = b.v * invDb, vc = c.v * invDc;
    const Vec3 na = a.normal * invDa, nb = b.normal * invDb, nc = c.normal * invDc;

    for (int y = minY; y <= maxY; ++y) {
        const float py = static_cast<float>(y) + 0.5f;
        const float px0 = static_cast<float>(minX) + 0.5f;

        float w0 = ((b.x - px0) * (c.y - py) - (b.y - py) * (c.x - px0)) * invArea;
        float w1 = ((c.x - px0) * (a.y - py) - (c.y - py) * (a.x - px0)) * invArea;

            float w1 =
                ((c.x - px) * (a.y - py) -
                 (c.y - py) * (a.x - px)) / area;

            float w2 = 1.0f - w0 - w1;

            if (w0 < 0 || w1 < 0 || w2 < 0) continue;

            int index = y * width + x;

            float z = w0 * a.z + w1 * b.z + w2 * c.z;

            if (z >= depthBuffer[index]) continue;

            // Perspective-correct interpolation.
            float denominator =
                w0 * invA +
                w1 * invB +
                w2 * invC;

            if (denominator <= 0.000001f) continue;

            float u = (
                w0 * a.u * invA +
                w1 * b.u * invB +
                w2 * c.u * invC
            ) / denominator;

            float v = (
                w0 * a.v * invA +
                w1 * b.v * invB +
                w2 * c.v * invC
            ) / denominator;

            FragmentInput input {
                {u, v},
                w0 * a.depth + w1 * b.depth + w2 * c.depth
            };

            // Execute the fragment shader.
            Color outputColor = fragmentShader(input, uniforms);

            depthBuffer[index] = z;
            pixels[index] = outputColor;
        }
    }
}

bool Framebuffer::savePPM(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);

    if (!file) return false;

    file << "P6\n" << width << " " << height << "\n255\n";

    for (const Color& color : pixels) {
        file.put(static_cast<char>(color.r));
        file.put(static_cast<char>(color.g));
        file.put(static_cast<char>(color.b));
    }

    return file.good();
}

int Framebuffer::getWidth() const {
    return width;
}

int Framebuffer::getHeight() const {
    return height;
}
std::size_t Framebuffer::getIndex(int x, int y) const {
    return static_cast<std::size_t>(y) * width + x;
}

const Color* Framebuffer::getPixels() const {
    return pixels.data();
}

const Color* Framebuffer::getPixel(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return nullptr;
    }

    return &pixels[getIndex(x, y)];
}
