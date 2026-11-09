#include "Framebuffer.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>

Framebuffer::Framebuffer(int width, int height)
    : width(width),
      height(height),
      pixels(width * height),
      depthBuffer(
          width * height,
          std::numeric_limits<float>::infinity()
      ) {}

void Framebuffer::clear(const Color& color) {
    std::fill(pixels.begin(), pixels.end(), color);

    std::fill(
        depthBuffer.begin(),
        depthBuffer.end(),
        std::numeric_limits<float>::infinity()
    );
}

void Framebuffer::setPixel(
    int x,
    int y,
    const Color& color
) {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return;
    }

    pixels[y * width + x] = color;
}

void Framebuffer::drawTriangle(
    int x0, int y0,
    int x1, int y1,
    int x2, int y2,
    const Color& color
) {
    float area = static_cast<float>(
        (x1 - x0) * (y2 - y0) -
        (y1 - y0) * (x2 - x0)
    );

    if (std::abs(area) < 0.0001f) return;

    int minX = std::max(0, std::min({x0, x1, x2}));
    int maxX = std::min(width - 1, std::max({x0, x1, x2}));

    int minY = std::max(0, std::min({y0, y1, y2}));
    int maxY = std::min(height - 1, std::max({y0, y1, y2}));

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            float w0 = static_cast<float>(
                (x1 - x) * (y2 - y) -
                (y1 - y) * (x2 - x)
            ) / area;

            float w1 = static_cast<float>(
                (x2 - x) * (y0 - y) -
                (y2 - y) * (x0 - x)
            ) / area;

            float w2 = 1.0f - w0 - w1;

            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                setPixel(x, y, color);
            }
        }
    }
}

void Framebuffer::drawTriangle3D(
    int x0, int y0, float z0,
    int x1, int y1, float z1,
    int x2, int y2, float z2,
    const Color& color
) {
    float area = static_cast<float>(
        (x1 - x0) * (y2 - y0) -
        (y1 - y0) * (x2 - x0)
    );

    if (std::abs(area) < 0.0001f) return;

    int minX = std::max(0, std::min({x0, x1, x2}));
    int maxX = std::min(width - 1, std::max({x0, x1, x2}));

    int minY = std::max(0, std::min({y0, y1, y2}));
    int maxY = std::min(height - 1, std::max({y0, y1, y2}));

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            float w0 = static_cast<float>(
                (x1 - x) * (y2 - y) -
                (y1 - y) * (x2 - x)
            ) / area;

            float w1 = static_cast<float>(
                (x2 - x) * (y0 - y) -
                (y2 - y) * (x0 - x)
            ) / area;

            float w2 = 1.0f - w0 - w1;

            if (w0 < 0 || w1 < 0 || w2 < 0) continue;

            float z = w0 * z0 + w1 * z1 + w2 * z2;
            int index = y * width + x;

            if (z < depthBuffer[index]) {
                depthBuffer[index] = z;
                pixels[index] = color;
            }
        }
    }
}

// Compatibility interface: uses a default texture fragment shader.
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

    FragmentShader shader = [](
        const FragmentInput& input,
        const ShaderUniforms& data
    ) -> Color {
        if (!data.texture ||
            data.textureWidth <= 0 ||
            data.textureHeight <= 0) {
            return Color(255, 0, 255);
        }

        const auto& tex = *data.texture;

        if (tex.size() < static_cast<std::size_t>(
            data.textureWidth * data.textureHeight
        )) {
            return Color(255, 0, 255);
        }

        float u = std::clamp(input.uv.u, 0.0f, 1.0f);
        float v = std::clamp(input.uv.v, 0.0f, 1.0f);

        int tx = std::min(
            data.textureWidth - 1,
            static_cast<int>(u * data.textureWidth)
        );

        int ty = std::min(
            data.textureHeight - 1,
            static_cast<int>(v * data.textureHeight)
        );

        return tex[ty * data.textureWidth + tx];
    };

    drawTexturedTriangle3D(a, b, c, shader, uniforms);
}

// Programmable fragment shader rasterization.
void Framebuffer::drawTexturedTriangle3D(
    const TexturedVertex& a,
    const TexturedVertex& b,
    const TexturedVertex& c,
    const FragmentShader& fragmentShader,
    const ShaderUniforms& uniforms
) {
    if (!fragmentShader ||
        a.depth <= 0 ||
        b.depth <= 0 ||
        c.depth <= 0) {
        return;
    }

    float area =
        (b.x - a.x) * (c.y - a.y) -
        (b.y - a.y) * (c.x - a.x);

    if (std::abs(area) < 0.0001f) return;

    int minX = std::max(
        0,
        static_cast<int>(std::floor(std::min({a.x, b.x, c.x})))
    );

    int maxX = std::min(
        width - 1,
        static_cast<int>(std::ceil(std::max({a.x, b.x, c.x})))
    );

    int minY = std::max(
        0,
        static_cast<int>(std::floor(std::min({a.y, b.y, c.y})))
    );

    int maxY = std::min(
        height - 1,
        static_cast<int>(std::ceil(std::max({a.y, b.y, c.y})))
    );

    float invA = 1.0f / a.depth;
    float invB = 1.0f / b.depth;
    float invC = 1.0f / c.depth;

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            float px = x + 0.5f;
            float py = y + 0.5f;

            float w0 =
                ((b.x - px) * (c.y - py) -
                 (b.y - py) * (c.x - px)) / area;

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
