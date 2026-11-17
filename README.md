# IrisGPU

A CPU-based 3D software renderer written from scratch in C++20. No OpenGL, no Vulkan,
just math, a framebuffer, and SDL2 to put pixels on screen.

![IrisGPU screenshot](docs/screenshot.png)

## Features

- Real matrix pipeline: `Mat4::perspective`, `Mat4::lookAt`, model/view/projection in clip space
- Programmable **vertex shader** and **fragment shader** (`std::function` callbacks)
- Homogeneous **frustum clipping** against all 6 planes (Sutherland-Hodgman in clip space)
- Backface culling via signed screen-space area
- Edge-function rasterizer with incremental barycentrics
- Perspective-correct interpolation of UVs and normals
- Z-buffer depth testing
- **Multithreaded rasterization** (screen split into bands, work-stealing across threads)
- Blinn-Phong lighting, distance fog, bilinear texture filtering with repeat wrap
- Mesh generators (cube, sphere, torus, plane) and a Wavefront **OBJ loader**
- Debug shading modes: lit, unlit, normals, depth
- Headless rendering (`--render`) and benchmarking (`--bench`)
- Unit tests + GitHub Actions CI

## Pipeline

```
MeshInstance -> model matrix
      |
Vertex shader (MVP -> clip space, normals -> view space)
      |
Frustum clipping (6 planes, clip space)
      |
Perspective divide -> Viewport transform
      |
Backface culling -> fan triangulation
      |
Tile/band rasterization (multithreaded) -> depth test
      |
Fragment shader (perspective-correct uv + normal)
      |
Framebuffer -> SDL2 window
```

## Build

```bash
sudo apt install build-essential cmake libsdl2-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Run

```bash
./build/irisgpu                       # demo scene
./build/irisgpu assets/pyramid.obj    # add your own OBJ model to the scene
./build/irisgpu --render shot.ppm     # render one frame headless
./build/irisgpu --bench 300           # benchmark (try --threads 1 to compare)
```

| Key | Action |
| --- | --- |
| W A S D | move |
| Q / E | down / up |
| Mouse | look |
| Shift | move faster |
| 1 / 2 / 3 / 4 | lit / unlit / normals / depth |
| B | toggle bilinear filtering |
| C | toggle backface culling |
| Space | pause animation |
| P | save screenshot (`.ppm`) |
| Esc | quit |

## Project layout

```
include/   Vec3, Vec4, Mat4, Camera, Shader, Mesh, Framebuffer, Renderer, ...
src/       Framebuffer (rasterizer), Renderer (pipeline), meshes, OBJ loader, main
tests/     Framebuffer + core (math, meshes, OBJ, renderer) tests
assets/    Example OBJ model
```

`irisgpu_core` is a static library with no SDL dependency, so the renderer can be
tested and run headless.

## Ideas for next steps

- Normal mapping and shadow mapping
- Mipmaps / trilinear filtering
- SIMD (or tiled) rasterization
- Texture loading from PNG (stb_image)
- Wireframe and point-light modes
