cmake_minimum_required(VERSION 3.16)

project(IrisGPU VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)

add_executable(irisgpu
    src/main.cpp
    src/Framebuffer.cpp
)

target_include_directories(irisgpu PRIVATE include)

target_link_libraries(irisgpu PRIVATE SDL2::SDL2)

target_compile_options(irisgpu PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
