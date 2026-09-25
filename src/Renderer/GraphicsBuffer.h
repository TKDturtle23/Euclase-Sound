#pragma once

#include <cstddef>
#include <cstdint>

namespace Euclase {

    enum class BufferUsage {
        Vertex,
        Index,
        Uniform,
        Storage,
        TransferSource,
        TransferDestination
    };

    enum class BufferMemory {
        GPUOnly,
        CPUToGPU,
        GPUToCPU
    };
    class GraphicsBuffer {
    public:
        virtual ~GraphicsBuffer() = default;

        virtual size_t GetSize() const = 0;

        virtual void* Map() = 0;
        virtual void Unmap() = 0;

        virtual void Write(
            const void* data,
            size_t size,
            size_t offset = 0
        ) = 0;
    };

}