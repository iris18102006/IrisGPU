cmake_minimum_required(VERSION 3.16)

project(IrisGPU VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)

# Renderer core: no SDL dependency, so it can be tested and used headless.
add_library(irisgpu_core STATIC
    src/Framebuffer.cpp
)

target_include_directories(irisgpu PRIVATE include)

target_link_libraries(irisgpu PRIVATE SDL2::SDL2)

target_compile_options(irisgpu PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

include(CTest)

if(BUILD_TESTING)
    add_executable(irisgpu_tests
        tests/FramebufferTests.cpp
        src/Framebuffer.cpp
    )

    target_include_directories(irisgpu_tests PRIVATE include)

    target_compile_options(irisgpu_tests PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

    add_test(
        NAME FramebufferTests
        COMMAND irisgpu_tests
    )
endif()
