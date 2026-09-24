#pragma once
#include <memory>

#include "GraphicsPipeline.h"

namespace Euclase {
    class GraphicsDevice {
    public:
        virtual ~GraphicsDevice() = default;

        virtual std::unique_ptr<GraphicsPipeline> CreatePipeline(
            const GraphicsPipelineDesc& desc
        ) = 0;
    };

}
