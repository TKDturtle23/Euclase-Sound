#ifndef EUCLASESOUND_EUCLASEGUI_H
#define EUCLASESOUND_EUCLASEGUI_H
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

#include "Font.h"
#include "vec.h"
#include "Renderer/GraphicsRenderer.h"
#include "platform/Event.h"
#include "platform/Platform.h"
namespace Euclase {
class Text;

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
    };
    class EuclaseGUI {
    public:
        static void Init(std::shared_ptr<Platform> platform, std::shared_ptr<GraphicsRenderer> renderer);
        static void Shutdown();

        static void BeginFrame();
        static void EndFrame();

  static void Begin(Window &window);
  static void End();

        void DrawRectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);

    private:
        static std::shared_ptr<Platform> platform;
        static std::shared_ptr<GraphicsRenderer> renderer;
        static std::unique_ptr<GraphicsPipeline> pipeline;
    };
} // Euclase

  static vec2 resizeStartMouse;
  static vec2 resizeStartPosition;
  static vec2 resizeStartSize;

};
}  // namespace Euclase
#endif
