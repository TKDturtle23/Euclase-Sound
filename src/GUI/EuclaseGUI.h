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


struct Box {
  vec2 location{};
  vec2 size{};
  vec4 color{};
};
  enum class WindowFlags : uint64_t{
    None = 0,
    Non_Resizable = 1 << 0,
    Non_Movable = 1 << 1,
    Non_Closable = 1 << 2,

  };
  struct Window {
    bool open = true;
    std::string title;
    vec2 position = { -1,-1};
    vec2 size = { -1,  -1};
    vec2 minSize = { 100, 100};
    uint64_t flags; // WindowFlags
    vec2 padding = { 8,  8};
    vec2 borderSize = { 4,  4};
    vec4 backgroundColor = { 0.2,  0.2,  0.2,  1};
  };
  enum class ResizeEdge {
    None,
    Left,
    Right,
    Top,
    Bottom,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};
class EuclaseGUI {
 public:
  static void Init(std::shared_ptr<Platform>,
                   std::shared_ptr<GraphicsRenderer>, std::string fontPath, uint32_t pixelHeight);
  static void Shutdown();
  static void BeginFrame();
  static void EndFrame();

  static void Begin(Window &window);
  static void End();

  static void Text(std::string_view text, float size, vec4 color = {1,1,1,1});
  static bool Button(std::string_view text, vec2 size = {100, 25}, vec4 color = {1,1,1,1});

private:
  static void DrawBox( Box& box);
  static float PixelsToUnits(float pixels, bool isWidth = true);
  static vec2 PixelsToUnits(vec2 pixels);

  static void MouseButtonCallback(int button, int action, int mods);
  static void MouseMoveCallback(double xpos, double ypos);
  static void MouseScrollCallback(double xoffset, double yoffset);
  static void KeyCallback(int key, int scancode, int action, int mods);

  static bool IsMousePressed(MouseButton button);

  static bool IsMouseReleased(MouseButton button);

  static ResizeEdge GetResizeEdge(const Window& window);

  static void ResizeSetCursor(ResizeEdge edge);

private:

  static std::shared_ptr<Platform> m_platform;
  static std::shared_ptr<GraphicsRenderer> m_renderer;
  static std::shared_ptr<GraphicsDevice> m_device;
  static std::unique_ptr<GraphicsPipeline> boxPipeline;
  static std::unique_ptr<GraphicsPipeline> textPipeline;
  static ShaderResource FontAtlas;
  static std::unique_ptr<Font> font;
  static std::shared_ptr<CommandBuffer> Buffer;
  // Input state
  static vec2 mousePosition;
  static vec2 mouseDelta;
  static vec2 mouseScroll;
  static bool previousMouseButtons[
      static_cast<size_t>(MouseButton::X2) + 1
  ];
  static bool mouseButtons[static_cast<size_t>(MouseButton::X2) + 1];
  static bool keys[static_cast<size_t>(KeyCode::Menu) + 1];


  static ResizeEdge resizeEdge;
  static Window* resizingWindow;

  static vec2 resizeStartMouse;
  static vec2 resizeStartPosition;
  static vec2 resizeStartSize;

};
}  // namespace Euclase
#endif
