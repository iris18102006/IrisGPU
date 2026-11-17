#pragma once

#include "Camera.hpp"
#include "Framebuffer.hpp"
#include "Mesh.hpp"
#include "Shader.hpp"

#include <cstddef>
#include <vector>

struct RenderStats {
    std::size_t trianglesSubmitted = 0;  // triangles passed to draw()
    std::size_t trianglesRasterized = 0; // triangles that survived culling and clipping (after fan split)
};

// Full pipeline: vertex shader -> clip space -> frustum clipping -> perspective divide
// -> viewport -> backface cull -> multithreaded rasterization -> fragment shader.
class Renderer {
public:
    explicit Renderer(Framebuffer& target, unsigned threads = 0);

    void setBackfaceCulling(bool enabled) { cullBackfaces = enabled; }
    bool backfaceCulling() const { return cullBackfaces; }
    unsigned threadCount() const { return threads; }

    void beginFrame(const Color& clearColor);

    // Queue a mesh instance. `fragmentShader` (and `vertexShader` if given) must stay alive until endFrame().
    void draw(
        const MeshInstance& instance,
        const Camera& camera,
        const ShaderUniforms& uniforms,
        const FragmentShader& fragmentShader,
        const VertexShader* vertexShader = nullptr
    );

