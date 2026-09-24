#pragma once

#include <cstdint>

namespace Euclase {

    enum class GraphicsAPI {
        Vulkan,
        OpenGL,
        DirectX12,
        Metal
    };

    enum class TextureFormat {
        Undefined,

        RGBA8,
        BGRA8,

        RGBA16F,

        Depth24Stencil8,
        Depth32F
    };

    struct Extent2D {
        uint32_t width = 0;
        uint32_t height = 0;
    };

    struct Color {
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        float a = 1.0f;
    };

}