# IrisGPU

A CPU-based 3D software rendering engine written from scratch in C++.

IrisGPU explores the fundamentals of computer graphics by implementing
a simplified 3D rendering pipeline without relying on OpenGL, Vulkan,
or other hardware graphics APIs.

## Features

- Custom framebuffer implementation
- Triangle rasterization
- Barycentric coordinate interpolation
- Depth buffering (Z-buffer)
- 3D vector mathematics
- 4x4 transformation matrices
- Perspective projection
- Viewport transformation
- Mesh representation and generation
- Real-time rendering through SDL2
- Vertex and fragment shader-style processing

## Architecture

The rendering pipeline:

3D Geometry
    ↓
Transformations
    ↓
Projection
    ↓
Viewport Transformation
    ↓
Triangle Rasterization
    ↓
Depth Testing
    ↓
Framebuffer
    ↓
SDL2 Display

## Technology Stack

- C++
- CMake
- SDL2
- Git

## Building

Install dependencies:

```bash
sudo apt install build-essential cmake libsdl2-dev
