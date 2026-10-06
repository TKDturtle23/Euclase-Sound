#ifndef EUCLASESOUND_EUCLASEGUI_H
#define EUCLASESOUND_EUCLASEGUI_H
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
typedef struct {
  vec2 location;
  vec2 size;
  vec4 Color;
  vec2 uv;
  vec2 uvSize;
} GlyphConstant;

enum class WindowFlags : uint64_t {
  None = 0,
  Non_Resizable = 1 << 0,
  Non_Movable = 1 << 1,
  Non_Closable = 1 << 2,
};
inline bool HasFlag(uint64_t flags, WindowFlags f) {
  return (flags & static_cast<uint64_t>(f)) != 0;
}

enum class DockingArea { None, Left, Right, Up, Down, Middle };

enum class ResizeEdge {
  None,
  Left,
  Right,
  Top,
  Bottom,
  TopLeft,
  TopRight,
  BottomLeft,
  BottomRight,
  Move
};

struct Window {
  bool open = true;
  std::string title;
  vec2 position = {-1, -1};  // -1 => auto. After Begin() this mirrors the dock layout.
  vec2 size = {-1, -1};
  vec2 minSize = {100, 100};
  uint64_t flags = 0;  // WindowFlags
  vec2 padding = {8, 8};
  vec2 borderSize = {4, 4};
  vec4 backgroundColor = {0.2, 0.2, 0.2, 1};

  // header (doubles as the tab bar when docked)
  bool HasHeader = true;
  float HeaderHeight = 50;
  vec4 HeaderColor = {0.15, 0.15, 0.15, 1};
  bool Draggable = true;
  bool Closeable = true;

  // docking
  bool Dockable = true;
  unsigned int TabMinimumSize = 100;
  // Where the window docks on its first appearance (relative to the application
  // window). None => starts floating. Middle is ignored.
  DockingArea InitialDock = DockingArea::None;
};

struct WindowRenderData {
  std::vector<Box> boxes;
  std::vector<GlyphConstant> texts;
};

struct CurrentWindow {
  Window* window = nullptr;
  WindowRenderData renderData;

  vec2 pointer{};
  vec2 size{};
  vec2 position{};
  vec2 contentSize{};

  // Layout state
  float lineStartY = 0;
  float lineEndX = 0;
  float lineHeight = 0;

  bool sameLine = false;
  float sameLineSpacing = 0;

  bool hidden = false;  // inactive tab / closed: widgets still run, output is discarded
};

// ---------------------------------------------------------------------------
// Dock tree
//
// Every window lives in a leaf. Leaves hold a tab list. Split nodes hold two
// children and a ratio. There is one "main" tree pinned to the application
// window; its leaf marked `central` is the area the application itself renders
// into, so docking to the application edge splits the main tree instead of
// stacking windows on top of each other. Windows that are not docked to the
// main tree live in "floating" trees (one root each, own position + size).
// Floating trees may contain splits/tabs too, so windows dock onto each other.
// ---------------------------------------------------------------------------
enum class DockAxis { None, Horizontal /* children side by side */, Vertical /* stacked */ };

struct DockNode {
  int id = 0;
  DockNode* parent = nullptr;
  std::unique_ptr<DockNode> child[2];
  DockAxis axis = DockAxis::None;  // None => leaf
  float ratio = 0.5f;              // share of child[0]
  float fixedPx = -1.0f;  // px size of the child that does NOT contain the central leaf; <0 => derive from ratio
  // leaf data
  std::vector<std::string> tabs;
  int active = 0;
  bool central = false;  // reserved for the application, holds no windows

  // computed by layout (authoritative for roots)
  vec2 pos{};
  vec2 size{};

  bool IsLeaf() const { return axis == DockAxis::None; }
};

struct HoverInfo {
  DockNode* root = nullptr;
  DockNode* leaf = nullptr;
  DockNode* splitter = nullptr;
  ResizeEdge edge = ResizeEdge::None;
  int tabIndex = -1;
  bool onBar = false;
};

enum class DragMode { None, PendingMove, MovingWindow, ResizeFloating, Splitter };

struct DragState {
  DragMode mode = DragMode::None;
  int nodeId = -1;
  std::string title;
  vec2 startMouse{};
  vec2 startPos{};
  vec2 startSize{};
  vec2 grab{};
  float startRatio = 0.5f;
  ResizeEdge edge = ResizeEdge::None;
};

struct DropTarget {
  DockingArea area = DockingArea::None;
  bool appEdge = false;
  int nodeId = -1;
};

struct TextBatch {
  size_t capacity = 0;  // glyphs
  std::vector<std::shared_ptr<GraphicsBuffer>> buffers;  // one per frame in flight
};

class EuclaseGUI {
 public:
  static void Init(std::shared_ptr<Platform>, std::shared_ptr<GraphicsRenderer>,
                   std::string fontPath, uint32_t pixelHeight,
                   int dockableArea /* how close you have to be to an app edge / how big the dock buttons are */);
  static void Shutdown();
  static void BeginFrame();
  static void EndFrame();

  // Returns false when the window is a hidden tab (or closed). Always call End().
  static bool Begin(Window &window);
  static void End();
  static void Render();

  static void Text(std::string_view text, float size, vec4 color = {1, 1, 1, 1});
  static void drawText(std::string_view text, float size, vec4 color);

  static bool Button(std::string_view text, vec2 size = {100, 25}, vec4 color = {1, 1, 1, 1});
  static void SameLine(float spacing = 0.0f);

  // Region (pixels) left over for the application after all docked windows.
  static void GetCentralArea(vec2 &position, vec2 &size);
  // True when the pointer is over GUI chrome/windows (or a drag is active).
  static bool IsMouseOverGUI();

 private:
  // drawing
  static void DrawBox(Box &box);
  static Box MakeBox(vec2 pos, vec2 size, vec4 color);
  static void AddOverlay(vec2 pos, vec2 size, vec4 color);
  static float PixelsToUnits(float pixels, bool isWidth = true);
  static vec2 PixelsToUnits(vec2 pixels);
  static void DrawTextAt(WindowRenderData &out, std::string_view text, vec2 pos, float size, vec4 color);
  static void DrawBatch(const WindowRenderData &data, TextBatch &batch);
  static void GrowTextPipeline(size_t neededGlyphs);
  static WindowRenderData BuildChrome(DockNode &root);

  // input
  static void MouseButtonCallback(int button, int action, int mods);
  static void MouseMoveCallback(double xpos, double ypos);
  static void MouseScrollCallback(double xoffset, double yoffset);
  static void KeyCallback(int key, int scancode, int action, int mods);
  static bool IsMousePressed(MouseButton button);
  static bool IsMouseReleased(MouseButton button);
  static ResizeEdge GetResizeEdge(vec2 position, vec2 size);
  static void ResizeSetCursor(ResizeEdge edge);
  static void ApplyHoverCursor();
  static void UpdateHover();
  static void HitNode(DockNode *n, vec2 p);
  static void HandleInput();
  static void BeginWindowMove(DockNode *leaf);
  static void UpdateDropTarget(DockNode *dragRoot);
  static void FinishMove();
  static bool IsRootResizable(DockNode *root);

  // dock tree
  static std::unique_ptr<DockNode> MakeNode();
  static std::unique_ptr<DockNode> &SlotOf(DockNode *n);
  static DockNode *FindNodeById(int id);
  static DockNode *FindLeafWithWindow(const std::string &title);
  static DockNode *LeafAt(DockNode *n, vec2 p);
  static void Visit(DockNode *n, const std::function<void(DockNode *)> &fn);
  static DockNode *CreateFloatingRoot(const std::string &title, vec2 pos, vec2 size);
  static void RegisterWindow(const Window &w);
  static void UndockWindow(const std::string &title);
  static void RemoveLeaf(DockNode *leaf);
  static void DockInto(DockNode *target, DockingArea area, const std::string &title, float newFraction);
  static void BringRootToFront(DockNode *root);
  static float AppEdgeFraction(DockingArea area, vec2 windowSize);

  // layout
  static void LayoutAll();
  static void LayoutNode(DockNode *n);
  static vec2 MinSize(const DockNode *n);
  static void SplitterRect(const DockNode *n, vec2 &pos, vec2 &size);
  static float BarHeight(const DockNode *leaf);
  static float TabWidth(const DockNode *leaf);
  static Window &W(const std::string &title);

 private:
  static std::shared_ptr<Platform> m_platform;
  static std::shared_ptr<GraphicsRenderer> m_renderer;
  static std::shared_ptr<GraphicsDevice> m_device;
  static std::unique_ptr<GraphicsPipeline> boxPipeline;
  static std::unique_ptr<GraphicsPipeline> textPipeline;
  static unsigned int PipelineTextBufferSize;  // in glyphs
  static GraphicsPipelineDesc textDesc;
  static ShaderResource FontAtlas;
  static ShaderResource TextData;
  static std::unique_ptr<Font> font;
  static float m_fontPixelHeight;
  static std::shared_ptr<CommandBuffer> Buffer;

  // Input state
  static vec2 mousePosition;
  static vec2 mouseDelta;
  static vec2 mouseScroll;
  static bool previousMouseButtons[static_cast<size_t>(MouseButton::X2) + 1];
  static bool mouseButtons[static_cast<size_t>(MouseButton::X2) + 1];
  static bool keys[static_cast<size_t>(KeyCode::Menu) + 1];

  // Interaction state
  static HoverInfo m_hover;
  static DragState m_drag;
  static DropTarget m_drop;
  static std::vector<Box> m_overlay;  // drawn on top of everything

  // Dock state
  static int m_dockableArea;
  static int m_nextNodeId;
  static vec2 m_viewport;
  static vec2 m_centralPos;
  static vec2 m_centralSize;
  static std::unique_ptr<DockNode> m_mainRoot;
  static std::vector<std::unique_ptr<DockNode>> m_floatingRoots;  // back -> front
  static std::unordered_map<std::string, vec2> m_floatSize;       // last floating size per window
  static std::unordered_set<std::string> m_seenThisFrame;

  // Windows
  static std::unordered_map<std::string, Window> m_Windows;
  static std::unordered_map<std::string, WindowRenderData> m_WindowRenderData;
  static std::unordered_map<std::string, TextBatch> m_TextBatches;
  static std::vector<TextBatch> m_ChromeBatches;
  static Window m_chromeWindow;
  static CurrentWindow currentWindow;
};
}  // namespace Euclase
#endif