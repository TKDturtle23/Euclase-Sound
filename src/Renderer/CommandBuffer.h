#pragma once

#include "GraphicsTypes.h"

namespace Euclase {
    class GraphicsPipeline;

    class CommandBuffer {
    public:
        virtual ~CommandBuffer() = default;

        CommandBuffer(const CommandBuffer&) = delete;
        CommandBuffer& operator=(const CommandBuffer&) = delete;

        virtual void Reset() = 0;
        virtual void Begin() = 0;
        virtual void End() = 0;

        virtual void Draw(unsigned int vertices) = 0;
        virtual void PushConstant(
            ShaderStage stage,
            uint32_t offset,
            const void* data,
            size_t size, GraphicsPipeline* pipeline
        ) = 0;

        virtual void PushResource(
            const ShaderResource& resource, GraphicsPipeline* pipeline
        ) = 0;
    protected:
        CommandBuffer() = default;
    };

}
