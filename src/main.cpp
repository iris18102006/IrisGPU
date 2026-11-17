#include "Camera.hpp"
#include "Framebuffer.hpp"
#include "MeshFactory.hpp"
#include "ObjLoader.hpp"
#include "Renderer.hpp"
#include "Shader.hpp"

#include <SDL2/SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

static_assert(sizeof(Color) == 3, "Color must be tightly packed RGB24 for SDL");

namespace {

constexpr int kWidth = 800;
constexpr int kHeight = 600;
constexpr float kPi = 3.14159265358979323846f;
const Color kBackground(15, 15, 25);

// ---------------------------------------------------------------------------
// Fragment shader: Blinn-Phong lighting + distance fog, with a few debug modes.
// ---------------------------------------------------------------------------
Color shadeFragment(const FragmentInput& in, const ShaderUniforms& u) {
    const Vec3 n = normalize(in.normal);

    auto toColor = [](float r, float g, float b) {
        auto c = [](float x) {
            return static_cast<std::uint8_t>(std::clamp(x, 0.0f, 1.0f) * 255.0f + 0.5f);
        };
        return Color(c(r), c(g), c(b));
    };

    switch (u.mode) {
        case ShadeMode::Normals:
            return toColor(n.x * 0.5f + 0.5f, n.y * 0.5f + 0.5f, n.z * 0.5f + 0.5f);

        case ShadeMode::Depth: {
            const float d = 1.0f - std::clamp(in.depth / 25.0f, 0.0f, 1.0f);
            return toColor(d, d, d);
        }

        case ShadeMode::TexturedFlat:
            return sampleTexture(u, in.uv);

        case ShadeMode::TexturedLit:
            break;
    }

    const Color base = sampleTexture(u, in.uv);

    const Vec3 lightDir = normalize(u.lightDirection);
    const float diffuse = std::max(dot(n, lightDir), 0.0f);

    // Directional approximation: the viewer looks down -Z in view space.
    const Vec3 half = normalize(lightDir + Vec3(0, 0, 1));
    const float specular = diffuse > 0.0f
        ? std::pow(std::max(dot(n, half), 0.0f), u.shininess) * u.specularStrength
        : 0.0f;

    const float light = u.ambient + (1.0f - u.ambient) * diffuse;

    const float fog = std::clamp((in.depth - 12.0f) / 30.0f, 0.0f, 1.0f);

    auto channel = [&](std::uint8_t base8, std::uint8_t bg8) {
        float c = static_cast<float>(base8) / 255.0f * light + specular;
        c = c * (1.0f - fog) + (static_cast<float>(bg8) / 255.0f) * fog;
        return static_cast<std::uint8_t>(std::clamp(c, 0.0f, 1.0f) * 255.0f + 0.5f);
    };

    return Color(
        channel(base.r, kBackground.r),
        channel(base.g, kBackground.g),
        channel(base.b, kBackground.b)
    );
}

// ---------------------------------------------------------------------------
// Scene
// ---------------------------------------------------------------------------
struct Scene {
    Mesh cube = createCubeMesh();
    Mesh sphere = createSphereMesh();
    Mesh torus = createTorusMesh();
    Mesh floor = createPlaneMesh(30.0f, 30.0f);
    Mesh custom;
    bool hasCustom = false;

    Texture cubeTex = createCheckerTexture(128, 128, 16, Color(245, 125, 65), Color(35, 35, 65));
    Texture sphereTex = createCheckerTexture(128, 128, 8, Color(80, 220, 200), Color(240, 240, 240));
    Texture torusTex = createCheckerTexture(128, 128, 8, Color(240, 90, 170), Color(70, 40, 120));
    Texture floorTex = createCheckerTexture(64, 64, 32, Color(90, 90, 105), Color(55, 55, 70));
    Texture customTex = createCheckerTexture(64, 64, 8, Color(230, 200, 90), Color(90, 70, 40));

    bool loadModel(const std::string& path) {
        std::string error;
        if (!loadObj(path, custom, &error)) {
            std::cerr << "Could not load '" << path << "': " << error << '\n';
            return false;
        }
        custom.normalizeToUnitSize();
        hasCustom = true;
        std::cout << "Loaded " << path << ": " << custom.vertices.size()
                  << " vertices, " << custom.triangles.size() << " triangles\n";
        return true;
    }

    void draw(Renderer& renderer, const Camera& camera, const ShaderUniforms& base,
              const FragmentShader& shader, float t) const {
        MeshInstance floorInst;
        floorInst.mesh = &floor;
        floorInst.texture = &floorTex;
        floorInst.position = Vec3(0, -1.5f, 0);
        renderer.draw(floorInst, camera, base, shader);

        MeshInstance cubeInst;
        cubeInst.mesh = &cube;
        cubeInst.texture = &cubeTex;
        cubeInst.position = Vec3(0, 0, -5);
        cubeInst.rotation = Vec3(t * 40.0f, t * 57.0f, 0);
        renderer.draw(cubeInst, camera, base, shader);

        MeshInstance sphereInst;
        sphereInst.mesh = &sphere;
        sphereInst.texture = &sphereTex;
        sphereInst.position = Vec3(-3.2f, std::sin(t * 1.5f) * 0.4f, -5);
        sphereInst.rotation = Vec3(0, t * 30.0f, 0);
        renderer.draw(sphereInst, camera, base, shader);

        MeshInstance torusInst;
        torusInst.mesh = &torus;
        torusInst.texture = &torusTex;
        torusInst.position = Vec3(3.2f, 0, -5);
        torusInst.rotation = Vec3(t * 50.0f, t * 25.0f, 0);
        renderer.draw(torusInst, camera, base, shader);

        if (hasCustom) {
            MeshInstance inst;
            inst.mesh = &custom;
            inst.texture = &customTex;
            inst.position = Vec3(0, 0.3f, -9);
            inst.rotation = Vec3(0, t * 35.0f, 0);
            inst.scale = Vec3(1.8f, 1.8f, 1.8f);
            renderer.draw(inst, camera, base, shader);
        }
    }
};

void renderFrame(Renderer& renderer, const Scene& scene, const Camera& camera,
                 ShaderUniforms uniforms, const FragmentShader& shader, float t) {
    // Light direction is defined in world space and brought into view space once per frame.
    const Vec3 worldLight = normalize(Vec3(0.5f, 0.9f, 0.6f));
    uniforms.lightDirection = normalize(camera.viewMatrix().transformDirection(worldLight));
    uniforms.time = t;

    renderer.beginFrame(kBackground);
    scene.draw(renderer, camera, uniforms, shader, t);
    renderer.endFrame();
}

Camera defaultCamera() {
    Camera camera;
    camera.position = Vec3(0, 1.2f, 2.5f);
    camera.pitch = -0.18f;
    return camera;
}

const char* modeName(ShadeMode m) {
    switch (m) {
        case ShadeMode::TexturedLit: return "lit";
        case ShadeMode::TexturedFlat: return "unlit";
        case ShadeMode::Normals: return "normals";
        case ShadeMode::Depth: return "depth";
    }
    return "?";
}

void printUsage() {
    std::cout <<
        "IrisGPU - CPU software renderer\n"
        "usage: irisgpu [model.obj] [--render out.ppm] [--bench frames] [--threads N]\n\n"
        "  model.obj          load an OBJ file into the scene\n"
        "  --render out.ppm   render one frame without a window and exit\n"
        "  --bench N          render N frames without a window and print timings\n"
        "  --threads N        rasterizer threads (default: all cores)\n\n"
        "controls: WASD move | Q/E down/up | mouse look | Shift fast | Space pause\n"
        "          1-4 shading mode | B bilinear | C backface culling | P screenshot | ESC quit\n";
}

}  // namespace

int main(int argc, char** argv) {
    std::string modelPath;
    std::string renderPath;
    int benchFrames = 0;
    unsigned threads = 0;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else if (arg == "--render" && i + 1 < argc) {
            renderPath = argv[++i];
        } else if (arg == "--bench" && i + 1 < argc) {
            benchFrames = std::atoi(argv[++i]);
        } else if (arg == "--threads" && i + 1 < argc) {
            threads = static_cast<unsigned>(std::max(1, std::atoi(argv[++i])));
        } else if (!arg.empty() && arg[0] != '-') {
            modelPath = arg;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n\n";
            printUsage();
            return 1;
        }
    }

    Scene scene;
    if (!modelPath.empty() && !scene.loadModel(modelPath)) return 1;

    Framebuffer framebuffer(kWidth, kHeight);
    Renderer renderer(framebuffer, threads);

    ShaderUniforms uniforms;
    const FragmentShader shader = shadeFragment;

    // ---- headless modes (no SDL needed at runtime) ----
    if (!renderPath.empty() || benchFrames > 0) {
        Camera camera = defaultCamera();

        if (benchFrames > 0) {
            using Clock = std::chrono::steady_clock;
            const auto start = Clock::now();

            for (int f = 0; f < benchFrames; ++f) {
                renderFrame(renderer, scene, camera, uniforms, shader, static_cast<float>(f) / 60.0f);
            }

            const double ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
            std::cout << benchFrames << " frames, " << renderer.threadCount() << " threads: "
                      << ms / benchFrames << " ms/frame (" << 1000.0 * benchFrames / ms << " fps), "
                      << renderer.stats().trianglesRasterized << " triangles rasterized\n";
        } else {
            renderFrame(renderer, scene, camera, uniforms, shader, 1.2f);
        }

        if (!renderPath.empty()) {
            if (!framebuffer.savePPM(renderPath)) {
                std::cerr << "Could not write " << renderPath << '\n';
                return 1;
            }
            std::cout << "Wrote " << renderPath << '\n';
        }

        return 0;
    }

    // ---- interactive mode ----
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL error: " << SDL_GetError() << '\n';
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "IrisGPU", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        kWidth, kHeight, SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* sdlRenderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!sdlRenderer) sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

    if (!sdlRenderer) {
        std::cerr << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture* displayTexture = SDL_CreateTexture(
        sdlRenderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, kWidth, kHeight
    );

    if (!displayTexture) {
        std::cerr << SDL_GetError() << '\n';
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Camera camera = defaultCamera();

    bool running = true;
    bool paused = false;
    float sceneTime = 0.0f;
    int screenshotCount = 0;

    SDL_SetRelativeMouseMode(SDL_TRUE);

    printUsage();
    std::cout << "\nRasterizer threads: " << renderer.threadCount() << '\n';

    Uint64 previous = SDL_GetPerformanceCounter();
    double titleTimer = 0.0;
    int titleFrames = 0;

    while (running) {
        const Uint64 now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(now - previous) / static_cast<float>(SDL_GetPerformanceFrequency());
        previous = now;
        dt = std::min(dt, 0.05f);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;

            if (event.type == SDL_MOUSEMOTION) {
                camera.yaw += static_cast<float>(event.motion.xrel) * 0.0025f;
                camera.pitch -= static_cast<float>(event.motion.yrel) * 0.0025f;
                camera.pitch = std::clamp(camera.pitch, -kPi / 2.0f + 0.01f, kPi / 2.0f - 0.01f);
            }

            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: running = false; break;
                    case SDLK_SPACE: paused = !paused; break;
                    case SDLK_1: uniforms.mode = ShadeMode::TexturedLit; break;
                    case SDLK_2: uniforms.mode = ShadeMode::TexturedFlat; break;
                    case SDLK_3: uniforms.mode = ShadeMode::Normals; break;
                    case SDLK_4: uniforms.mode = ShadeMode::Depth; break;
                    case SDLK_b: uniforms.bilinear = !uniforms.bilinear; break;
                    case SDLK_c: renderer.setBackfaceCulling(!renderer.backfaceCulling()); break;
                    case SDLK_p: {
                        const std::string name = "screenshot_" + std::to_string(screenshotCount++) + ".ppm";
                        if (framebuffer.savePPM(name)) std::cout << "Saved " << name << '\n';
                        break;
                    }
                    default: break;
                }
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
