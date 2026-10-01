#pragma once
#include <cstdint>

namespace Euclase {
    class GraphicsTexture {
    public:
        virtual ~GraphicsTexture() = default;

        virtual uint32_t GetWidth() const = 0;

        virtual uint32_t GetHeight() const = 0;
    };
}
