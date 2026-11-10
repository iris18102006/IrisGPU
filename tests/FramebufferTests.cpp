
#include "Framebuffer.hpp"

#include <iostream>
#include <stdexcept>

bool testValidDimensions() {
    Framebuffer framebuffer(800, 600);

    return framebuffer.getWidth() == 800 &&
           framebuffer.getHeight() == 600 &&
           framebuffer.getPixels() != nullptr;
}

bool testZeroWidth() {
    try {
        Framebuffer framebuffer(0, 600);
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

bool testZeroHeight() {
    try {
        Framebuffer framebuffer(800, 0);
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

bool testNegativeDimensions() {
    try {
        Framebuffer framebuffer(-800, 600);
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

bool testPixelWrite() {
    Framebuffer framebuffer(10, 10);
    framebuffer.setPixel(5, 5, Color(255, 0, 0));

    const Color& pixel = framebuffer.getPixels()[55];

    return pixel.r == 255 && pixel.g == 0 && pixel.b == 0;
}

bool testPixelOutsideBounds() {
    Framebuffer framebuffer(10, 10);
    framebuffer.clear(Color(0, 0, 0));

    framebuffer.setPixel(-1, 0, Color(255, 0, 0));
    framebuffer.setPixel(10, 0, Color(255, 0, 0));
    framebuffer.setPixel(0, -1, Color(255, 0, 0));
    framebuffer.setPixel(0, 10, Color(255, 0, 0));

    for (int i = 0; i < 100; ++i) {
        const Color& pixel = framebuffer.getPixels()[i];

        if (pixel.r != 0 || pixel.g != 0 || pixel.b != 0) {
            return false;
        }
    }

    return true;
}

bool testClear() {
    Framebuffer framebuffer(10, 10);

    framebuffer.setPixel(2, 3, Color(255, 0, 0));
    framebuffer.clear(Color(0, 255, 0));

    for (int i = 0; i < 100; ++i) {
        const Color& pixel = framebuffer.getPixels()[i];

        if (pixel.r != 0 || pixel.g != 255 || pixel.b != 0) {
            return false;
        }
    }

    return true;
}

bool testTriangleRendering() {
    Framebuffer framebuffer(20, 20);
    framebuffer.clear(Color(0, 0, 0));

    framebuffer.drawTriangle(
        2, 2, 15, 2, 2, 15,
        Color(255, 0, 0)
    );

    const Color& pixel = framebuffer.getPixels()[4 * 20 + 4];

    return pixel.r == 255 && pixel.g == 0 && pixel.b == 0;
}

bool testDegenerateTriangle() {
    Framebuffer framebuffer(20, 20);
    framebuffer.clear(Color(0, 0, 0));

    framebuffer.drawTriangle(
        2, 2, 5, 5, 8, 8,
        Color(255, 0, 0)
    );

    for (int i = 0; i < 400; ++i) {
        const Color& pixel = framebuffer.getPixels()[i];

        if (pixel.r != 0 || pixel.g != 0 || pixel.b != 0) {
            return false;
        }
    }

    return true;
}

bool testDepthTesting() {
    Framebuffer framebuffer(20, 20);
    framebuffer.clear(Color(0, 0, 0));

    framebuffer.drawTriangle3D(
        2, 2, 0.2f,
        15, 2, 0.2f,
        2, 15, 0.2f,
        Color(255, 0, 0)
    );

    framebuffer.drawTriangle3D(
        2, 2, 0.8f,
        15, 2, 0.8f,
        2, 15, 0.8f,
        Color(0, 0, 255)
    );

    const Color& pixel = framebuffer.getPixels()[4 * 20 + 4];

    return pixel.r == 255 && pixel.g == 0 && pixel.b == 0;
}

int main() {
    int passed = 0;
    int failed = 0;

    auto runTest = [&](const char* name, bool (*test)()) {
        if (test()) {
            std::cout << "[PASS] " << name << '\n';
            ++passed;
        } else {
            std::cout << "[FAIL] " << name << '\n';
            ++failed;
        }
    };

    runTest("Valid framebuffer dimensions", testValidDimensions);
    runTest("Zero width rejection", testZeroWidth);
    runTest("Zero height rejection", testZeroHeight);
    runTest("Negative dimensions rejection", testNegativeDimensions);
    runTest("Pixel write", testPixelWrite);
    runTest("Out-of-bounds pixel protection", testPixelOutsideBounds);
    runTest("Framebuffer clearing", testClear);
    runTest("Triangle rendering", testTriangleRendering);
    runTest("Degenerate triangle rejection", testDegenerateTriangle);
    runTest("Depth testing", testDepthTesting);

    std::cout << "\nResults: " << passed
              << " passed, " << failed << " failed\n";

    return failed == 0 ? 0 : 1;
}
