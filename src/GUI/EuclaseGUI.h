//
// Created by Loyal on 9/24/26.
//

#ifndef EUCLASESOUND_EUCLASEGUI_H
#define EUCLASESOUND_EUCLASEGUI_H
#include <cstdint>
#include <memory>
#include "platform/Platform.h"
#include "Renderer/GraphicsRenderer.h"
namespace Euclase {

    struct vec2 {
        float x, y;
    };
    struct vec4 {
        float x, y, z, w;
    };
    struct Box {
        vec2 location;
        vec2 size;
        vec4 color;
        vec2 frameSize;
    };
    class EuclaseGUI {
    public:
        static void Init(std::shared_ptr<Platform> platform, GraphicsRenderer* renderer);
        static void Shutdown();

        static void BeginFrame();
        static void EndFrame();

        static void Draw();

        static void DrawRectangle(uint32_t x, uint32_t y, float width, float height, uint32_t color);

    private:
        static std::shared_ptr<Platform> platform;
        static GraphicsRenderer* renderer;
        static std::unique_ptr<GraphicsPipeline> pipeline;
        static ShaderConstant constant;
        static std::shared_ptr<CommandBuffer> buffer;
    };
} // Euclase

#endif //EUCLASESOUND_EUCLASEGUI_H
