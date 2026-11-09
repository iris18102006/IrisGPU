
#include "Framebuffer.hpp"
#include "Shader.hpp"
#include "Vec3.hpp"
#include "Mat4.hpp"
#include "Projection.hpp"
#include "Viewport.hpp"

#include <SDL2/SDL.h>

#include <iostream>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

constexpr float PI = 3.14159265358979323846f;
constexpr float NEAR_PLANE = 0.1f;
constexpr float FAR_PLANE = 100.0f;
constexpr float CLIP_EPSILON = 0.0001f;

struct ClipVertex {
    Vec3 position;
    Vec2 uv;
};

Vec3 subtract(const Vec3& a, const Vec3& b) {
    return Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

Vec3 add(const Vec3& a, const Vec3& b) {
    return Vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vec3 multiply(const Vec3& v, float s) {
    return Vec3(v.x * s, v.y * s, v.z * s);
}

float dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

Vec3 normalize(const Vec3& v) {
    float length = std::sqrt(dot(v, v));

    if (length < 0.00001f) {
        return Vec3(0, 0, 0);
    }

    return multiply(v, 1.0f / length);
}

void worldToCamera(
    const Vec3& point,
    const Vec3& camera,
    float yaw,
    float pitch,
    Vec3& result
) {
    Vec3 relative = subtract(point, camera);

    float cy = std::cos(yaw);
    float sy = std::sin(yaw);

    float x1 = cy * relative.x - sy * relative.z;
    float z1 = sy * relative.x + cy * relative.z;

    float cp = std::cos(pitch);
    float sp = std::sin(pitch);

    result = Vec3(
        x1,
        cp * relative.y + sp * z1,
        -sp * relative.y + cp * z1
    );
}

ClipVertex interpolate(
    const ClipVertex& a,
    const ClipVertex& b,
    float t
) {
    return {
        Vec3(
            a.position.x + (b.position.x - a.position.x) * t,
            a.position.y + (b.position.y - a.position.y) * t,
            a.position.z + (b.position.z - a.position.z) * t
        ),
        {
            a.uv.u + (b.uv.u - a.uv.u) * t,
            a.uv.v + (b.uv.v - a.uv.v) * t
        }
    };
}

std::vector<ClipVertex> clipAgainstPlane(
    const std::vector<ClipVertex>& polygon,
    float planeZ,
    bool keepLess
) {
    std::vector<ClipVertex> output;

    if (polygon.empty()) {
        return output;
    }

    auto inside = [&](const ClipVertex& vertex) {
        if (keepLess) {
            return vertex.position.z <= planeZ;
        }

        return vertex.position.z >= planeZ;
    };

    ClipVertex previous = polygon.back();
    bool previousInside = inside(previous);

    for (const ClipVertex& current : polygon) {
        bool currentInside = inside(current);

        if (currentInside != previousInside) {
            float denominator =
                current.position.z - previous.position.z;

            if (std::abs(denominator) > 0.000001f) {
                float t =
                    (planeZ - previous.position.z) / denominator;

                output.push_back(interpolate(previous, current, t));
            }
        }

        if (currentInside) {
            output.push_back(current);
        }

        previous = current;
        previousInside = currentInside;
    }

    return output;
}

std::vector<Color> createCheckerTexture(int width, int height) {
    std::vector<Color> texture(width * height);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            bool alternate = ((x / 16) + (y / 16)) % 2 == 0;

            texture[y * width + x] = alternate
                ? Color(245, 125, 65)
                : Color(35, 35, 65);
        }
    }

    return texture;
}

int main() {
    const int width = 800;
    const int height = 600;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL error: " << SDL_GetError() << '\n';
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "IrisGPU - Programmable Software Renderer",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer) {
        renderer = SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_SOFTWARE
        );
    }

    if (!renderer) {
        std::cerr << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture* displayTexture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );

    if (!displayTexture) {
        std::cerr << SDL_GetError() << '\n';
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Framebuffer framebuffer(width, height);

    const int textureWidth = 128;
    const int textureHeight = 128;

    std::vector<Color> texture =
        createCheckerTexture(textureWidth, textureHeight);

    std::array<Vec3, 8> vertices = {
        Vec3(-1, -1, -1), Vec3(1, -1, -1),
        Vec3(1, 1, -1), Vec3(-1, 1, -1),
        Vec3(-1, -1, 1), Vec3(1, -1, 1),
        Vec3(1, 1, 1), Vec3(-1, 1, 1)
    };

    std::array<std::array<int, 3>, 12> triangles = {{
        {{0, 1, 2}}, {{0, 2, 3}},
        {{5, 4, 7}}, {{5, 7, 6}},
        {{4, 0, 3}}, {{4, 3, 7}},
        {{1, 5, 6}}, {{1, 6, 2}},
        {{3, 2, 6}}, {{3, 6, 7}},
        {{4, 5, 1}}, {{4, 1, 0}}
    }};

    std::array<std::array<Vec2, 3>, 12> uvs = {{
        {{{0,1}, {1,1}, {1,0}}},
        {{{0,1}, {1,0}, {0,0}}},
        {{{0,1}, {1,1}, {1,0}}},
        {{{0,1}, {1,0}, {0,0}}},
        {{{0,1}, {1,1}, {1,0}}},
        {{{0,1}, {1,0}, {0,0}}},
        {{{0,1}, {1,1}, {1,0}}},
        {{{0,1}, {1,0}, {0,0}}},
        {{{0,1}, {1,1}, {1,0}}},
        {{{0,1}, {1,0}, {0,0}}},
        {{{0,1}, {1,1}, {1,0}}},
        {{{0,1}, {1,0}, {0,0}}}
    }};

    Vec3 cameraPosition(0, 0, 0);

    float yaw = 0.0f;
    float pitch = 0.0f;
    float angle = 0.0f;

    const float movementSpeed = 4.0f;
    const float mouseSensitivity = 0.0025f;
    const float aspectRatio = static_cast<float>(width) / height;

    // ==========================================
    // IRISGPU VERTEX SHADER
    // ==========================================

    VertexShader vertexShader =
        [](const Vertex& input, const ShaderUniforms& uniforms)
            -> VertexOutput {

        Vec3 worldPosition =
            uniforms.model.transformPoint(input.position);

        Vec3 cameraSpacePosition;

        worldToCamera(
            worldPosition,
            uniforms.cameraPosition,
            uniforms.yaw,
            uniforms.pitch,
            cameraSpacePosition
        );

        return {
            cameraSpacePosition,
            input.uv
        };
    };

    // ==========================================
    // IRISGPU FRAGMENT SHADER
    // ==========================================

    FragmentShader fragmentShader =
        [](const FragmentInput& input, const ShaderUniforms& uniforms)
            -> Color {

        if (
            uniforms.texture == nullptr ||
            uniforms.textureWidth <= 0 ||
            uniforms.textureHeight <= 0
        ) {
            return Color(255, 0, 255);
        }

        float u = std::clamp(input.uv.u, 0.0f, 0.9999f);
        float v = std::clamp(input.uv.v, 0.0f, 0.9999f);

        int tx = static_cast<int>(u * uniforms.textureWidth);
        int ty = static_cast<int>(v * uniforms.textureHeight);

        const Color& base =
            (*uniforms.texture)[ty * uniforms.textureWidth + tx];

        float wave = std::sin(
            uniforms.time * 3.0f + input.uv.u * 12.0f
        );

        float brightness = 0.75f + 0.25f * wave;

        auto shade = [brightness](std::uint8_t channel) {
            return static_cast<std::uint8_t>(
                std::clamp(
                    static_cast<float>(channel) * brightness,
                    0.0f,
                    255.0f
                )
            );
        };

        return Color(
            shade(base.r),
            shade(base.g),
            shade(base.b)
        );
    };

    bool running = true;
    SDL_Event event;

    SDL_SetRelativeMouseMode(SDL_TRUE);

    Uint64 previousTime = SDL_GetPerformanceCounter();

    std::cout << "IrisGPU: Programmable shaders enabled!\n";
    std::cout << "Vertex shader: ACTIVE\n";
    std::cout << "Fragment shader: ACTIVE\n";
    std::cout << "WASD: Move | Mouse: Look | Q/E: Vertical | ESC: Exit\n";

    while (running) {
        Uint64 currentTime = SDL_GetPerformanceCounter();

        float deltaTime = static_cast<float>(
            currentTime - previousTime
        ) / static_cast<float>(SDL_GetPerformanceFrequency());

        previousTime = currentTime;
        deltaTime = std::min(deltaTime, 0.05f);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (
                event.type == SDL_KEYDOWN &&
                event.key.keysym.sym == SDLK_ESCAPE
            ) {
                running = false;
            }

            if (event.type == SDL_MOUSEMOTION) {
                yaw += event.motion.xrel * mouseSensitivity;
                pitch -= event.motion.yrel * mouseSensitivity;

                pitch = std::clamp(
                    pitch,
                    -PI / 2.0f + 0.01f,
                    PI / 2.0f - 0.01f
                );
            }
        }

        const Uint8* keys = SDL_GetKeyboardState(nullptr);

        float movement = movementSpeed * deltaTime;

        Vec3 forward = normalize(Vec3(
            std::sin(yaw) * std::cos(pitch),
            std::sin(pitch),
            -std::cos(yaw) * std::cos(pitch)
        ));

        Vec3 right = normalize(Vec3(
            std::cos(yaw),
            0,
            std::sin(yaw)
        ));

        if (keys[SDL_SCANCODE_W])
            cameraPosition = add(cameraPosition, multiply(forward, movement));

        if (keys[SDL_SCANCODE_S])
            cameraPosition = subtract(cameraPosition, multiply(forward, movement));

        if (keys[SDL_SCANCODE_D])
            cameraPosition = add(cameraPosition, multiply(right, movement));

        if (keys[SDL_SCANCODE_A])
            cameraPosition = subtract(cameraPosition, multiply(right, movement));

        if (keys[SDL_SCANCODE_E])
            cameraPosition.y += movement;

        if (keys[SDL_SCANCODE_Q])
            cameraPosition.y -= movement;

        framebuffer.clear(Color(15, 15, 25));

        Mat4 model =
            Mat4::translation(Vec3(0, 0, -5)) *
            Mat4::rotationY(angle) *
            Mat4::rotationX(angle * 0.7f);

        // Shader uniforms are updated every frame.
        ShaderUniforms uniforms;

        uniforms.model = model;
        uniforms.cameraPosition = cameraPosition;
        uniforms.yaw = yaw;
        uniforms.pitch = pitch;
        uniforms.time = angle;

        uniforms.texture = &texture;
        uniforms.textureWidth = textureWidth;
        uniforms.textureHeight = textureHeight;

        std::array<Vec3, 8> worldVertices;
        std::array<Vec3, 8> viewVertices;

        // Execute the vertex shader for every cube vertex.
        for (int i = 0; i < 8; ++i) {
            worldVertices[i] = model.transformPoint(vertices[i]);

            Vertex input{
                vertices[i],
                {0.0f, 0.0f}
            };

            VertexOutput output = vertexShader(input, uniforms);

            viewVertices[i] = output.position;
        }

        Vec3 cubeCenter = model.transformPoint(Vec3(0, 0, 0));

        for (int i = 0; i < 12; ++i) {
            int ia = triangles[i][0];
            int ib = triangles[i][1];
            int ic = triangles[i][2];

            Vec3 a = worldVertices[ia];
            Vec3 b = worldVertices[ib];
            Vec3 c = worldVertices[ic];

            Vec3 normal = normalize(
                cross(subtract(b, a), subtract(c, a))
            );

            Vec3 faceCenter(
                (a.x + b.x + c.x) / 3.0f,
                (a.y + b.y + c.y) / 3.0f,
                (a.z + b.z + c.z) / 3.0f
            );

            Vec3 outward = subtract(faceCenter, cubeCenter);

            if (dot(normal, outward) < 0.0f) {
                normal = multiply(normal, -1.0f);
            }

            Vec3 viewA = viewVertices[ia];
            Vec3 viewB = viewVertices[ib];
            Vec3 viewC = viewVertices[ic];

            Vec3 viewCenter(
                (viewA.x + viewB.x + viewC.x) / 3.0f,
                (viewA.y + viewB.y + viewC.y) / 3.0f,
                (viewA.z + viewB.z + viewC.z) / 3.0f
            );

            Vec3 viewNormal;
            worldToCamera(
                add(faceCenter, normal),
                cameraPosition,
                yaw,
                pitch,
                viewNormal
            );

            Vec3 viewNormalDirection = normalize(
                subtract(viewNormal, viewCenter)
            );

            Vec3 viewDirection = normalize(
                multiply(viewCenter, -1.0f)
            );

            // Backface culling.
            if (dot(viewNormalDirection, viewDirection) <= 0.0f) {
                continue;
            }

            std::vector<ClipVertex> polygon = {
                {viewA, uvs[i][0]},
                {viewB, uvs[i][1]},
                {viewC, uvs[i][2]}
            };

            // Near-plane clipping.
            polygon = clipAgainstPlane(
                polygon,
                -NEAR_PLANE - CLIP_EPSILON,
                true
            );

            // Far-plane clipping.
            polygon = clipAgainstPlane(
                polygon,
                -FAR_PLANE + CLIP_EPSILON,
                false
            );

            if (polygon.size() < 3) {
                continue;
            }

            std::vector<TexturedVertex> projected;
            projected.reserve(polygon.size());

            for (const ClipVertex& vertex : polygon) {
                Vec3 point = Projection::perspective(
                    vertex.position,
                    90.0f,
                    aspectRatio,
                    NEAR_PLANE,
                    FAR_PLANE
                );

                Vec3 screen = Viewport::toScreen(
                    point,
                    width,
                    height
                );

                projected.push_back({
                    screen.x,
                    screen.y,
                    screen.z,
                    -vertex.position.z,
                    vertex.uv.u,
                    vertex.uv.v
                });
            }

            // Triangulate clipped polygon and execute fragment shader.
            for (std::size_t j = 1; j + 1 < projected.size(); ++j) {
                framebuffer.drawTexturedTriangle3D(
                    projected[0],
                    projected[j],
                    projected[j + 1],
                    fragmentShader,
                    uniforms
                );
            }
        }

        SDL_UpdateTexture(
            displayTexture,
            nullptr,
            framebuffer.getPixels(),
            width * static_cast<int>(sizeof(Color))
        );

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, displayTexture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        angle += 0.6f * deltaTime;

        if (angle >= 2.0f * PI) {
            angle -= 2.0f * PI;
        }
    }

    SDL_SetRelativeMouseMode(SDL_FALSE);

    SDL_DestroyTexture(displayTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "IrisGPU: Renderer stopped.\n";

    return 0;
}
