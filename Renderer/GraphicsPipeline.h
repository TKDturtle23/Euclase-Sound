//
// Created by lstuart9 on 9/24/2026.
//

#ifndef EUCLASESOUND_GRAPHICSPIPELINE_H
#define EUCLASESOUND_GRAPHICSPIPELINE_H
#include <string>

#include "CommandBuffer.h"
#include "GraphicsTypes.h"

namespace Euclase {
    struct GraphicsPipelineDesc {
        std::string vertexShader;
        std::string fragmentShader;

        TextureFormat colorFormat = TextureFormat::RGBA8;
        TextureFormat depthFormat = TextureFormat::Undefined;

        bool depthTest = false;
        bool depthWrite = false;

        bool blending = false;
    };
    class GraphicsPipeline {
    public:
        virtual ~GraphicsPipeline() = default;

        virtual void Bind(std::shared_ptr<CommandBuffer> buffer) = 0;

    protected:

    };
}


#endif //EUCLASESOUND_GRAPHICSPIPELINE_H
