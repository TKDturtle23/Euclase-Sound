#pragma once
#include <memory>

#include "GraphicsBuffer.h"
#include "GraphicsTexture.h"
#include "GraphicsPipeline.h"

namespace Euclase {
    class GraphicsDevice {
    public:
        virtual ~GraphicsDevice() = default;

        virtual std::unique_ptr<GraphicsPipeline> CreatePipeline(
            const GraphicsPipelineDesc& desc
        ) = 0;
        virtual std::shared_ptr<GraphicsBuffer> CreateBuffer(size_t size, BufferUsage usage, BufferMemory memory) = 0;
        virtual std::shared_ptr<GraphicsTexture> CreateTexture(uint32_t width, uint32_t height, TextureFormat format, const void* pixels, size_t size) = 0;
    };

}
