#include "EuclaseGUI.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace Euclase {
namespace util {
std::string ReadFile(const std::string &path) {
  std::ifstream file(path);

  if (!file)
    throw std::runtime_error("Failed to open file: " + path);

  std::stringstream buffer;
  buffer << file.rdbuf();

  return buffer.str();
}
}  // namespace util

namespace {
constexpr float kSplitter = 6.0f;      // visible gap between split children
constexpr float kSplitterGrab = 3.0f;  // extra grab margin on each side
constexpr float kResizeBorder = 14.0f;
constexpr float kDragThreshold = 4.0f;
constexpr float kMaxTabWidth = 240.0f;
constexpr float kDockMinW = 120.0f;
constexpr float kDockMinH = 100.0f;
constexpr float kCentralMin = 200.0f;

bool Inside(vec2 p, vec2 pos, vec2 size) {
  return p.x >= pos.x && p.x < pos.x + size.x && p.y >= pos.y && p.y < pos.y + size.y;
}
  bool ContainsCentral(const DockNode *n) {
  if (!n) return false;
  return n->central || ContainsCentral(n->child[0].get()) || ContainsCentral(n->child[1].get());
}
}  // namespace

// ---------------------------------------------------------------------------
// statics
// ---------------------------------------------------------------------------
std::shared_ptr<Platform> EuclaseGUI::m_platform;
std::shared_ptr<GraphicsRenderer> EuclaseGUI::m_renderer;
std::shared_ptr<GraphicsDevice> EuclaseGUI::m_device;

std::unique_ptr<GraphicsPipeline> EuclaseGUI::boxPipeline;
std::unique_ptr<GraphicsPipeline> EuclaseGUI::textPipeline;
unsigned int EuclaseGUI::PipelineTextBufferSize;
GraphicsPipelineDesc EuclaseGUI::textDesc;
ShaderResource EuclaseGUI::FontAtlas;
ShaderResource EuclaseGUI::TextData;
std::unique_ptr<Font> EuclaseGUI::font;
float EuclaseGUI::m_fontPixelHeight = 32.0f;
std::shared_ptr<CommandBuffer> EuclaseGUI::Buffer;

vec2 EuclaseGUI::mousePosition{};
vec2 EuclaseGUI::mouseDelta{};
vec2 EuclaseGUI::mouseScroll{};
  vec2 EuclaseGUI::mouseScrollDelta{};
bool EuclaseGUI::previousMouseButtons[static_cast<size_t>(MouseButton::X2) + 1]{};
bool EuclaseGUI::mouseButtons[static_cast<size_t>(MouseButton::X2) + 1]{};
bool EuclaseGUI::keys[static_cast<size_t>(KeyCode::Menu) + 1]{};

HoverInfo EuclaseGUI::m_hover;
DragState EuclaseGUI::m_drag;
DropTarget EuclaseGUI::m_drop;
std::vector<Box> EuclaseGUI::m_overlay;

int EuclaseGUI::m_dockableArea;
int EuclaseGUI::m_nextNodeId = 1;
vec2 EuclaseGUI::m_viewport{};
vec2 EuclaseGUI::m_centralPos{};
vec2 EuclaseGUI::m_centralSize{};
std::unique_ptr<DockNode> EuclaseGUI::m_mainRoot;
std::vector<std::unique_ptr<DockNode>> EuclaseGUI::m_floatingRoots;
std::unordered_map<std::string, vec2> EuclaseGUI::m_floatSize;
std::unordered_set<std::string> EuclaseGUI::m_seenThisFrame;

std::unordered_map<std::string, Window> EuclaseGUI::m_Windows;
std::unordered_map<std::string, WindowRenderData> EuclaseGUI::m_WindowRenderData;
std::unordered_map<std::string, TextBatch> EuclaseGUI::m_TextBatches;
std::vector<TextBatch> EuclaseGUI::m_ChromeBatches;
Window EuclaseGUI::m_chromeWindow;
CurrentWindow EuclaseGUI::currentWindow;

// ---------------------------------------------------------------------------
// small helpers
// ---------------------------------------------------------------------------
Window &EuclaseGUI::W(const std::string &title) { return m_Windows[title]; }

Box EuclaseGUI::MakeBox(vec2 pos, vec2 size, vec4 color) {
  Box b;
  b.location = PixelsToUnits(pos);
  b.size = PixelsToUnits(size);
  b.color = color;
  return b;
}

void EuclaseGUI::AddOverlay(vec2 pos, vec2 size, vec4 color) {
  m_overlay.push_back(MakeBox(pos, size, color));
}

  float EuclaseGUI::BarHeight(const DockNode *n) {
  // Allow rendering tab headers if the leaf contains tabs
  if (!n || !n->IsLeaf() || n->tabs.empty())
    return 0.0f;

  const int idx = std::clamp(n->active, 0, static_cast<int>(n->tabs.size()) - 1);
  const Window &w = W(n->tabs[idx]);
  return (n->tabs.size() > 1 || w.HasHeader) ? w.HeaderHeight : 0.0f;
}

float EuclaseGUI::TabWidth(const DockNode *n) {
  if (n->tabs.size() <= 1)
    return n->size.x;
  return std::min(kMaxTabWidth, n->size.x / static_cast<float>(n->tabs.size()));
}

// ---------------------------------------------------------------------------
// dock tree
// ---------------------------------------------------------------------------
std::unique_ptr<DockNode> EuclaseGUI::MakeNode() {
  auto n = std::make_unique<DockNode>();
  n->id = m_nextNodeId++;
  return n;
}

void EuclaseGUI::Visit(DockNode *n, const std::function<void(DockNode *)> &fn) {
  if (!n)
    return;
  fn(n);
  Visit(n->child[0].get(), fn);
  Visit(n->child[1].get(), fn);
}

std::unique_ptr<DockNode> &EuclaseGUI::SlotOf(DockNode *n) {
  if (n->parent)
    return n->parent->child[n->parent->child[0].get() == n ? 0 : 1];
  if (n == m_mainRoot.get())
    return m_mainRoot;
  for (auto &r : m_floatingRoots)
    if (r.get() == n)
      return r;
  throw std::logic_error("EuclaseGUI: orphaned dock node");
}

DockNode *EuclaseGUI::FindNodeById(int id) {
  DockNode *found = nullptr;
  auto check = [&](DockNode *n) {
    if (!found && n->id == id)
      found = n;
  };
  Visit(m_mainRoot.get(), check);
  for (auto &r : m_floatingRoots)
    Visit(r.get(), check);
  return found;
}

DockNode *EuclaseGUI::FindLeafWithWindow(const std::string &title) {
  DockNode *res = nullptr;
  auto check = [&](DockNode *n) {
    if (!res && n->IsLeaf() &&
        std::find(n->tabs.begin(), n->tabs.end(), title) != n->tabs.end())
      res = n;
  };
  Visit(m_mainRoot.get(), check);
  for (auto &r : m_floatingRoots)
    Visit(r.get(), check);
  return res;
}

DockNode *EuclaseGUI::LeafAt(DockNode *n, vec2 p) {
  while (n && !n->IsLeaf()) {
    DockNode *a = n->child[0].get();
    DockNode *b = n->child[1].get();
    n = Inside(p, a->pos, a->size) ? a : (Inside(p, b->pos, b->size) ? b : nullptr);
  }
  return n;
}

DockNode *EuclaseGUI::CreateFloatingRoot(const std::string &title, vec2 pos, vec2 size) {
  auto n = MakeNode();
  n->tabs.push_back(title);
  n->pos = pos;
  n->size = size;
  DockNode *raw = n.get();
  m_floatingRoots.push_back(std::move(n));  // back == front of z-order
  return raw;
}

void EuclaseGUI::RegisterWindow(const Window &w) {
  const vec2 size = (w.size.x > 0 && w.size.y > 0) ? w.size : vec2{400.0f, 300.0f};
  const float cascade = 40.0f * static_cast<float>(m_floatingRoots.size() % 8);
  const vec2 pos = (w.position.x >= 0 && w.position.y >= 0) ? w.position
                                                          : vec2{100.0f + cascade, 100.0f + cascade};
  m_floatSize[w.title] = size;

  if (w.InitialDock != DockingArea::None && w.InitialDock != DockingArea::Middle) {
    DockInto(m_mainRoot.get(), w.InitialDock, w.title, AppEdgeFraction(w.InitialDock, size));
  } else {
    CreateFloatingRoot(w.title, pos, size);
  }
}

// Removes a leaf that has no tabs left. Its sibling takes the parent's place.
void EuclaseGUI::RemoveLeaf(DockNode *leaf) {
  DockNode *parent = leaf->parent;
  if (!parent) {
    if (leaf == m_mainRoot.get())
      return;  // main tree always keeps its root
    m_floatingRoots.erase(
        std::remove_if(m_floatingRoots.begin(), m_floatingRoots.end(),
                       [&](const std::unique_ptr<DockNode> &r) { return r.get() == leaf; }),
        m_floatingRoots.end());
    return;
  }
  const int idx = parent->child[0].get() == leaf ? 0 : 1;
  std::unique_ptr<DockNode> sibling = std::move(parent->child[1 - idx]);
  sibling->parent = parent->parent;
  sibling->pos = parent->pos;
  sibling->size = parent->size;
  std::unique_ptr<DockNode> &slot = SlotOf(parent);
  slot = std::move(sibling);  // destroys `parent` and `leaf`
}

void EuclaseGUI::UndockWindow(const std::string &title) {
  DockNode *leaf = FindLeafWithWindow(title);
  if (!leaf)
    return;
  auto it = std::find(leaf->tabs.begin(), leaf->tabs.end(), title);
  const int idx = static_cast<int>(it - leaf->tabs.begin());
  leaf->tabs.erase(it);
  if (leaf->tabs.empty()) {
    RemoveLeaf(leaf);
    return;
  }
  if (idx < leaf->active)
    --leaf->active;
  leaf->active = std::clamp(leaf->active, 0, static_cast<int>(leaf->tabs.size()) - 1);
}

// Docks `title` into `target`: Middle => new tab, otherwise split the target.
// `newFraction` is the share of the target's space the new window gets.
// Docking into the main root splits the whole application window.
  void EuclaseGUI::DockInto(DockNode *target, DockingArea area, const std::string &title,
                           float newFraction) {
  if (!target) return;

  if (area == DockingArea::Middle) {
    DockNode *leafTarget = target;

    // Traverse down to find a leaf target inside branch nodes
    while (leafTarget && !leafTarget->IsLeaf()) {
      if (ContainsCentral(leafTarget->child[1].get())) {
        leafTarget = leafTarget->child[1].get();
      } else {
        leafTarget = leafTarget->child[0].get();
      }
    }

    if (leafTarget) {
      leafTarget->tabs.push_back(title);
      leafTarget->active = static_cast<int>(leafTarget->tabs.size()) - 1;
    }
    return;
  }

  if (area == DockingArea::None)
    return;

  const bool newFirst = (area == DockingArea::Left || area == DockingArea::Up);
  DockNode *parent = target->parent;
  std::unique_ptr<DockNode> &slot = SlotOf(target);
  std::unique_ptr<DockNode> existing = std::move(slot);

  auto split = MakeNode();
  split->axis = (area == DockingArea::Left || area == DockingArea::Right) ? DockAxis::Horizontal
                                                                          : DockAxis::Vertical;
  split->pos = existing->pos;
  split->size = existing->size;
  split->parent = parent;
  split->ratio = newFirst ? newFraction : 1.0f - newFraction;

  auto leaf = MakeNode();
  leaf->tabs.push_back(title);
  leaf->parent = split.get();
  existing->parent = split.get();

  if (newFirst) {
    split->child[0] = std::move(leaf);
    split->child[1] = std::move(existing);
  } else {
    split->child[0] = std::move(existing);
    split->child[1] = std::move(leaf);
  }
  slot = std::move(split);
}


void EuclaseGUI::BringRootToFront(DockNode *root) {
  auto it = std::find_if(m_floatingRoots.begin(), m_floatingRoots.end(),
                         [&](const std::unique_ptr<DockNode> &r) { return r.get() == root; });
  if (it != m_floatingRoots.end() && it + 1 != m_floatingRoots.end())
    std::rotate(it, it + 1, m_floatingRoots.end());
}

float EuclaseGUI::AppEdgeFraction(DockingArea a, vec2 size) {
  const bool horiz = (a == DockingArea::Left || a == DockingArea::Right);
  const float frac = horiz ? size.x / std::max(1.0f, m_viewport.x)
                           : size.y / std::max(1.0f, m_viewport.y);
  return std::clamp(frac, 0.15f, 0.4f);
}

bool EuclaseGUI::IsRootResizable(DockNode *root) {
  if (root && root->IsLeaf() && !root->tabs.empty())
    return !HasFlag(W(root->tabs[0]).flags, WindowFlags::Non_Resizable);
  return true;
}

// ---------------------------------------------------------------------------
// layout
// ---------------------------------------------------------------------------
  vec2 EuclaseGUI::MinSize(const DockNode *n) {
  if (n->IsLeaf()) {
    if (n->central)
      return vec2{kCentralMin, kCentralMin};

    vec2 m{kDockMinW, kDockMinH};
    for (const auto &t : n->tabs) {  // a tab group must fit its largest member
      auto it = m_Windows.find(t);
      if (it == m_Windows.end())
        continue;
      const Window &w = it->second;
      // minSize is the content extent measured in End() (it already includes the tab bar,
      // top border and padding); add the right/bottom border and padding
      m.x = std::max(m.x, w.minSize.x + w.padding.x + w.borderSize.x);
      m.y = std::max(m.y, w.minSize.y + w.padding.y + w.borderSize.y);
    }
    return m;
  }

  const vec2 a = MinSize(n->child[0].get());
  const vec2 b = MinSize(n->child[1].get());
  if (n->axis == DockAxis::Horizontal)
    return vec2{a.x + b.x + kSplitter, std::max(a.y, b.y)};
  return vec2{std::max(a.x, b.x), a.y + b.y + kSplitter};
}

void EuclaseGUI::SplitterRect(const DockNode *n, vec2 &pos, vec2 &size) {
  const DockNode *a = n->child[0].get();
  if (n->axis == DockAxis::Horizontal) {
    pos = {a->pos.x + a->size.x, n->pos.y};
    size = {kSplitter, n->size.y};
  } else {
    pos = {n->pos.x, a->pos.y + a->size.y};
    size = {n->size.x, kSplitter};
  }
}

  void EuclaseGUI::LayoutNode(DockNode *n) {
  if (n->central) {
    m_centralPos = n->pos;
    m_centralSize = n->size;
  }
  if (n->IsLeaf())
    return;

  DockNode *a = n->child[0].get();
  DockNode *b = n->child[1].get();
  const bool horiz = n->axis == DockAxis::Horizontal;

  const vec2 minA = MinSize(a);
  const vec2 minB = MinSize(b);
  const float avail = std::max(0.0f, (horiz ? n->size.x : n->size.y) - kSplitter);
  const float mA = horiz ? minA.x : minA.y;
  const float mB = horiz ? minB.x : minB.y;

  const bool cA = ContainsCentral(a);
  const bool cB = ContainsCentral(b);

  float sizeA;
  if (cA != cB) {
    // one side hosts the application: the other keeps its pixel size
    if (n->fixedPx < 0.0f && avail > 0.0f)
      n->fixedPx = (cA ? 1.0f - n->ratio : n->ratio) * avail;
    const float fixedMin = cA ? mB : mA;
    const float flexMin = cA ? mA : mB;
    float fixedSize = std::max(0.0f, n->fixedPx);  // preference is never overwritten by clamping
    if (avail >= fixedMin + flexMin)
      fixedSize = std::clamp(fixedSize, fixedMin, avail - flexMin);
    else
      fixedSize = std::min(fixedSize, avail);
    sizeA = cA ? avail - fixedSize : fixedSize;
  } else {
    sizeA = avail * n->ratio;
    if (avail >= mA + mB)
      sizeA = std::clamp(sizeA, mA, avail - mB);
  }

  const float sizeB = avail - sizeA;
  if (avail > 0.0f)
    n->ratio = sizeA / avail;

  if (horiz) {
    a->pos = n->pos;
    a->size = {sizeA, n->size.y};
    b->pos = {n->pos.x + sizeA + kSplitter, n->pos.y};
    b->size = {sizeB, n->size.y};
  } else {
    a->pos = n->pos;
    a->size = {n->size.x, sizeA};
    b->pos = {n->pos.x, n->pos.y + sizeA + kSplitter};
    b->size = {n->size.x, sizeB};
  }
  LayoutNode(a);
  LayoutNode(b);
}
void EuclaseGUI::LayoutAll() {
  m_mainRoot->pos = {0, 0};
  m_mainRoot->size = m_viewport;
  LayoutNode(m_mainRoot.get());

  for (auto &r : m_floatingRoots) {
    const vec2 mn = MinSize(r.get());
    r->size = {std::max(r->size.x, mn.x), std::max(r->size.y, mn.y)};

    // keep a grabbable piece of the root on screen
    const float lo = 80.0f - r->size.x;
    r->pos.x = std::clamp(r->pos.x, lo, std::max(lo, m_viewport.x - 80.0f));
    r->pos.y = std::clamp(r->pos.y, 0.0f, std::max(0.0f, m_viewport.y - 40.0f));

    if (r->IsLeaf() && r->tabs.size() == 1)
      m_floatSize[r->tabs[0]] = r->size;
    LayoutNode(r.get());
  }
}

void EuclaseGUI::GetCentralArea(vec2 &position, vec2 &size) {
  position = m_centralPos;
  size = m_centralSize;
}

  bool EuclaseGUI::IsMouseOverGUI() {
  if (m_drag.mode != DragMode::None)
    return true;

  // Pass mouse input through to the app ONLY if the central leaf has no tabs
  return m_hover.root && !(m_hover.leaf && m_hover.leaf->central && m_hover.leaf->tabs.empty());
}

// ---------------------------------------------------------------------------
// input helpers
// ---------------------------------------------------------------------------
bool EuclaseGUI::IsMousePressed(MouseButton button) {
  const size_t index = static_cast<size_t>(button);
  return mouseButtons[index] && !previousMouseButtons[index];
}

bool EuclaseGUI::IsMouseReleased(MouseButton button) {
  const size_t index = static_cast<size_t>(button);
  return !mouseButtons[index] && previousMouseButtons[index];
}

ResizeEdge EuclaseGUI::GetResizeEdge(vec2 pos, vec2 size) {
  const float left = pos.x;
  const float right = pos.x + size.x;
  const float top = pos.y;
  const float bottom = pos.y + size.y;

  const bool onLeft = mousePosition.x >= left - kResizeBorder && mousePosition.x <= left + kResizeBorder;
  const bool onRight = mousePosition.x >= right - kResizeBorder && mousePosition.x <= right + kResizeBorder;
  const bool onTop = mousePosition.y >= top - kResizeBorder && mousePosition.y <= top + kResizeBorder;
  const bool onBottom = mousePosition.y >= bottom - kResizeBorder && mousePosition.y <= bottom + kResizeBorder;

  if (onLeft && onTop) return ResizeEdge::TopLeft;
  if (onRight && onTop) return ResizeEdge::TopRight;
  if (onLeft && onBottom) return ResizeEdge::BottomLeft;
  if (onRight && onBottom) return ResizeEdge::BottomRight;
  if (onLeft) return ResizeEdge::Left;
  if (onRight) return ResizeEdge::Right;
  if (onTop) return ResizeEdge::Top;
  if (onBottom) return ResizeEdge::Bottom;
  return ResizeEdge::None;
}

void EuclaseGUI::ResizeSetCursor(ResizeEdge edge) {
  switch (edge) {
    case ResizeEdge::TopLeft: m_platform->SetCursor(CursorShape::ResizeTopLeft); break;
    case ResizeEdge::TopRight: m_platform->SetCursor(CursorShape::ResizeTopRight); break;
    case ResizeEdge::BottomLeft: m_platform->SetCursor(CursorShape::ResizeBottomLeft); break;
    case ResizeEdge::BottomRight: m_platform->SetCursor(CursorShape::ResizeBottomRight); break;
    case ResizeEdge::Left:
    case ResizeEdge::Right: m_platform->SetCursor(CursorShape::ResizeHorizontal); break;
    case ResizeEdge::Top:
    case ResizeEdge::Bottom: m_platform->SetCursor(CursorShape::ResizeVertical); break;
    case ResizeEdge::Move: m_platform->SetCursor(CursorShape::Move); break;
    default: m_platform->SetCursor(CursorShape::Default); break;
  }
}

void EuclaseGUI::ApplyHoverCursor() {
  if (m_hover.splitter) {
    m_platform->SetCursor(m_hover.splitter->axis == DockAxis::Horizontal
                              ? CursorShape::ResizeHorizontal
                              : CursorShape::ResizeVertical);
    return;
  }
  if (m_hover.edge != ResizeEdge::None && IsRootResizable(m_hover.root)) {
    ResizeSetCursor(m_hover.edge);
    return;
  }
  if (m_hover.leaf && m_hover.leaf->central)
    return;  // the application owns the cursor here
  ResizeSetCursor(m_hover.onBar ? ResizeEdge::Move : ResizeEdge::None);
}

// Fills m_hover.leaf / splitter / tab info for a point inside `n`'s rect.
void EuclaseGUI::HitNode(DockNode *n, vec2 p) {
  while (n) {
    if (n->IsLeaf()) {
      m_hover.leaf = n;
      const float barH = BarHeight(n);
      m_hover.onBar = barH > 0.0f && p.y < n->pos.y + barH;
      if (m_hover.onBar) {
        int idx = static_cast<int>((p.x - n->pos.x) / std::max(1.0f, TabWidth(n)));
        if (idx < 0 || idx >= static_cast<int>(n->tabs.size()))
          idx = n->active;  // empty part of the bar drags the active tab
        m_hover.tabIndex = idx;
      }
      return;
    }
    DockNode *a = n->child[0].get();
    DockNode *b = n->child[1].get();
    vec2 sp, ss;
    SplitterRect(n, sp, ss);
    if (n->axis == DockAxis::Horizontal) {
      sp.x -= kSplitterGrab;
      ss.x += 2 * kSplitterGrab;
    } else {
      sp.y -= kSplitterGrab;
      ss.y += 2 * kSplitterGrab;
    }
    if (Inside(p, sp, ss)) {
      m_hover.splitter = n;
      return;
    }
    n = Inside(p, a->pos, a->size) ? a : (Inside(p, b->pos, b->size) ? b : nullptr);
  }
}

void EuclaseGUI::UpdateHover() {
  m_hover = {};
  const vec2 p = mousePosition;

  for (int i = static_cast<int>(m_floatingRoots.size()) - 1; i >= 0; --i) {
    DockNode *r = m_floatingRoots[i].get();
    const vec2 ep = {r->pos.x - kResizeBorder, r->pos.y - kResizeBorder};
    const vec2 es = {r->size.x + 2 * kResizeBorder, r->size.y + 2 * kResizeBorder};
    if (!Inside(p, ep, es))
      continue;
    m_hover.root = r;
    if (Inside(p, r->pos, r->size))
      HitNode(r, p);
    m_hover.edge = GetResizeEdge(r->pos, r->size);
    if (m_hover.edge != ResizeEdge::None && IsRootResizable(r)) {
      m_hover.leaf = nullptr;
      m_hover.splitter = nullptr;
      m_hover.onBar = false;
    } else {
      m_hover.edge = ResizeEdge::None;
    }
    return;
  }

  if (Inside(p, {0, 0}, m_viewport)) {
    m_hover.root = m_mainRoot.get();
    HitNode(m_mainRoot.get(), p);
  }
}

// ---------------------------------------------------------------------------
// dragging / docking interaction
// ---------------------------------------------------------------------------
void EuclaseGUI::BeginWindowMove(DockNode *leaf) {
  const std::string title = m_drag.title;
  DockNode *root = nullptr;

  if (!leaf->parent && leaf != m_mainRoot.get() && leaf->tabs.size() == 1) {
    // lone floating window: just move it
    root = leaf;
    m_drag.grab = {m_drag.startMouse.x - leaf->pos.x, m_drag.startMouse.y - leaf->pos.y};
  } else {
    vec2 size{400.0f, 300.0f};
    auto it = m_floatSize.find(title);
    if (it != m_floatSize.end())
      size = it->second;
    size = {std::max(size.x, kDockMinW), std::max(size.y, kDockMinH)};

    const float barH = std::max(BarHeight(leaf), 1.0f);
    m_drag.grab = {
        std::clamp(m_drag.startMouse.x - leaf->pos.x, 20.0f, std::max(20.0f, size.x - 20.0f)),
        std::clamp(m_drag.startMouse.y - leaf->pos.y, 0.0f, barH)};

    UndockWindow(title);  // may destroy `leaf`
    root = CreateFloatingRoot(title, {mousePosition.x - m_drag.grab.x, mousePosition.y - m_drag.grab.y}, size);
  }
  m_drag.mode = DragMode::MovingWindow;
  m_drag.nodeId = root->id;
}

// Works out what the dragged window would dock to and fills m_overlay with the
// edge strips, dock buttons and a translucent preview of the result.
void EuclaseGUI::UpdateDropTarget(DockNode *dragRoot) {
  m_drop = {};
  m_overlay.clear();
  if (!W(m_drag.title).Dockable)
    return;

  const float vw = m_viewport.x;
  const float vh = m_viewport.y;
  const float a = static_cast<float>(m_dockableArea);
  const vec2 p = mousePosition;
  const vec4 stripColor{0.1f, 0.1f, 0.5f, 0.25f};
  const vec4 previewColor{0.2f, 0.3f, 0.9f, 0.35f};

  // Application outer edge strips
  AddOverlay({0, 0}, {a, vh}, stripColor);
  AddOverlay({vw - a, 0}, {a, vh}, stripColor);
  AddOverlay({0, 0}, {vw, a}, stripColor);
  AddOverlay({0, vh - a}, {vw, a}, stripColor);

  DockingArea edgeArea = DockingArea::None;
  if (p.x <= a) edgeArea = DockingArea::Left;
  else if (p.x >= vw - a) edgeArea = DockingArea::Right;
  else if (p.y <= a) edgeArea = DockingArea::Up;
  else if (p.y >= vh - a) edgeArea = DockingArea::Down;

  if (edgeArea != DockingArea::None) {
    m_drop.area = edgeArea;
    m_drop.appEdge = true;
    const float f = AppEdgeFraction(edgeArea, dragRoot->size);
    switch (edgeArea) {
      case DockingArea::Left:  AddOverlay({0, 0}, {f * vw, vh}, previewColor); break;
      case DockingArea::Right: AddOverlay({vw - f * vw, 0}, {f * vw, vh}, previewColor); break;
      case DockingArea::Up:    AddOverlay({0, 0}, {vw, f * vh}, previewColor); break;
      case DockingArea::Down:  AddOverlay({0, vh - f * vh}, {vw, f * vh}, previewColor); break;
      default: break;
    }
    return;
  }

  // Node under pointer: floating trees first, then main root
  DockNode *leaf = nullptr;
  bool hitFloating = false;
  for (int i = static_cast<int>(m_floatingRoots.size()) - 1; i >= 0; --i) {
    DockNode *r = m_floatingRoots[i].get();
    if (r == dragRoot)
      continue;
    if (Inside(p, r->pos, r->size)) {
      leaf = LeafAt(r, p);
      hitFloating = true;
      break;
    }
  }
  if (!hitFloating)
    leaf = LeafAt(m_mainRoot.get(), p);
  if (!leaf)
    return;

  const float minDim = std::min(leaf->size.x, leaf->size.y);
  const float S = std::clamp(a, 16.0f, std::max(16.0f, minDim / 4.0f));
  const float step = S + 6.0f;
  if (leaf->size.x < step * 3 || leaf->size.y < step * 3)
    return;

  const vec2 c = {leaf->pos.x + leaf->size.x * 0.5f, leaf->pos.y + leaf->size.y * 0.5f};
  struct Btn { DockingArea area; float dx, dy; };
  const Btn btns[] = {
    {DockingArea::Middle, 0, 0},
    {DockingArea::Left,  -step, 0},
    {DockingArea::Right,  step, 0},
    {DockingArea::Up,     0, -step},
    {DockingArea::Down,   0, step}
  };

  for (const Btn &b : btns) {
    // Allow Middle docking for non-central nodes OR central nodes if intended
    if (Inside(p, {c.x + b.dx - S * 0.5f, c.y + b.dy - S * 0.5f}, {S, S})) {
      m_drop.area = b.area;
      m_drop.nodeId = leaf->id;
    }
  }

  if (m_drop.area != DockingArea::None) {
    const vec2 lp = leaf->pos, ls = leaf->size;
    switch (m_drop.area) {
      case DockingArea::Left:   AddOverlay(lp, {ls.x * 0.5f, ls.y}, previewColor); break;
      case DockingArea::Right:  AddOverlay({lp.x + ls.x * 0.5f, lp.y}, {ls.x * 0.5f, ls.y}, previewColor); break;
      case DockingArea::Up:     AddOverlay(lp, {ls.x, ls.y * 0.5f}, previewColor); break;
      case DockingArea::Down:   AddOverlay({lp.x, lp.y + ls.y * 0.5f}, {ls.x, ls.y * 0.5f}, previewColor); break;
      case DockingArea::Middle: AddOverlay(lp, ls, previewColor); break;
      default: break;
    }
  }

  for (const Btn &b : btns) {
    const bool hot = (b.area == m_drop.area);
    AddOverlay({c.x + b.dx - S * 0.5f, c.y + b.dy - S * 0.5f}, {S, S},
               hot ? vec4{0.3f, 0.5f, 1.0f, 0.95f} : vec4{0.25f, 0.25f, 0.25f, 0.85f});
  }
}
void EuclaseGUI::FinishMove() {
  if (m_drop.area != DockingArea::None) {
    const std::string title = m_drag.title;
    DockNode *dragRoot = FindNodeById(m_drag.nodeId);
    const vec2 size = dragRoot ? dragRoot->size : vec2{400.0f, 300.0f};

    DockNode *target = m_drop.appEdge ? m_mainRoot.get() : FindNodeById(m_drop.nodeId);
    if (target) {
      UndockWindow(title);  // destroys the floating root it was riding in
      const float fraction = m_drop.appEdge ? AppEdgeFraction(m_drop.area, size) : 0.5f;
      DockInto(target, m_drop.area, title, fraction);
    }
  }
  m_overlay.clear();
  m_drop = {};
}

void EuclaseGUI::HandleInput() {
  const bool down = mouseButtons[static_cast<size_t>(MouseButton::Left)];
  const bool pressed = IsMousePressed(MouseButton::Left);
  const vec2 mouse = mousePosition;


  switch (m_drag.mode) {
    case DragMode::None: {
      if (m_hover.root)
        ApplyHoverCursor();
      if (!pressed || !m_hover.root)
        return;
      if (m_hover.leaf && m_hover.leaf->central && !m_hover.onBar)
        return;  // central content belongs to the application

      BringRootToFront(m_hover.root);

      if (m_hover.splitter) {
        m_drag = {};
        m_drag.mode = DragMode::Splitter;
        m_drag.nodeId = m_hover.splitter->id;
        m_drag.startMouse = mouse;
        m_drag.startSize = m_hover.splitter->child[0]->size;  // pixel size of child A at press
      } else if (m_hover.edge != ResizeEdge::None && IsRootResizable(m_hover.root)) {
        m_drag = {};
        m_drag.mode = DragMode::ResizeFloating;
        m_drag.nodeId = m_hover.root->id;
        m_drag.startMouse = mouse;
        m_drag.startPos = m_hover.root->pos;
        m_drag.startSize = m_hover.root->size;
        m_drag.edge = m_hover.edge;
      } else if (m_hover.leaf && m_hover.onBar && m_hover.tabIndex >= 0) {
        DockNode *leaf = m_hover.leaf;
        leaf->active = m_hover.tabIndex;
        const std::string &title = leaf->tabs[leaf->active];
        const Window &w = W(title);
        if (w.Draggable && !HasFlag(w.flags, WindowFlags::Non_Movable)) {
          m_drag = {};
          m_drag.mode = DragMode::PendingMove;
          m_drag.nodeId = leaf->id;
          m_drag.title = title;
          m_drag.startMouse = mouse;
        }
      }
      return;
    }

    case DragMode::PendingMove: {
      DockNode *leaf = FindNodeById(m_drag.nodeId);
      if (!down || !leaf || !leaf->IsLeaf()) {
        m_drag = {};
        return;
      }
      auto it = std::find(leaf->tabs.begin(), leaf->tabs.end(), m_drag.title);
      if (it == leaf->tabs.end()) {
        m_drag = {};
        return;
      }
      const int cur = static_cast<int>(it - leaf->tabs.begin());
      const float barH = BarHeight(leaf);
      const bool inBand = mouse.x >= leaf->pos.x - 10 && mouse.x <= leaf->pos.x + leaf->size.x + 10 &&
                          mouse.y >= leaf->pos.y - 10 && mouse.y <= leaf->pos.y + barH + 10;
      const float dist = std::hypot(mouse.x - m_drag.startMouse.x, mouse.y - m_drag.startMouse.y);

      if (leaf->tabs.size() > 1 && inBand) {
        // reorder tabs inside the bar
        const int idx = std::clamp(static_cast<int>((mouse.x - leaf->pos.x) / std::max(1.0f, TabWidth(leaf))),
                                   0, static_cast<int>(leaf->tabs.size()) - 1);
        if (idx != cur) {
          std::string t = leaf->tabs[cur];
          leaf->tabs.erase(leaf->tabs.begin() + cur);
          leaf->tabs.insert(leaf->tabs.begin() + idx, t);
          leaf->active = idx;
        }
      } else if (leaf->tabs.size() > 1 || dist > kDragThreshold) {
        BeginWindowMove(leaf);  // tore the tab out of its bar / moved a lone window
      }
      return;
    }

    case DragMode::MovingWindow: {
      DockNode *root = FindNodeById(m_drag.nodeId);
      if (!root) {
        m_drag = {};
        m_overlay.clear();
        return;
      }
      root->pos = {mouse.x - m_drag.grab.x, mouse.y - m_drag.grab.y};
      UpdateDropTarget(root);  // recomputed on the release frame too
      if (!down) {
        FinishMove();
        m_drag = {};
      } else {
        m_platform->SetCursor(CursorShape::Move);
      }
      return;
    }

    case DragMode::ResizeFloating: {
      DockNode *root = FindNodeById(m_drag.nodeId);
      if (!root || !down) {
        m_drag = {};
        return;
      }
      ResizeSetCursor(m_drag.edge);

      vec2 mn = MinSize(root);
      if (root->IsLeaf() && !root->tabs.empty()) {
        const Window &w = W(root->tabs[0]);
        mn = {std::max(mn.x, w.minSize.x), std::max(mn.y, w.minSize.y)};
      }

      const float dx = mouse.x - m_drag.startMouse.x;
      const float dy = mouse.y - m_drag.startMouse.y;
      const ResizeEdge e = m_drag.edge;
      const bool L = e == ResizeEdge::Left || e == ResizeEdge::TopLeft || e == ResizeEdge::BottomLeft;
      const bool R = e == ResizeEdge::Right || e == ResizeEdge::TopRight || e == ResizeEdge::BottomRight;
      const bool T = e == ResizeEdge::Top || e == ResizeEdge::TopLeft || e == ResizeEdge::TopRight;
      const bool B = e == ResizeEdge::Bottom || e == ResizeEdge::BottomLeft || e == ResizeEdge::BottomRight;

      vec2 np = m_drag.startPos;
      vec2 ns = m_drag.startSize;
      if (L) { ns.x -= dx; np.x += dx; }
      if (R) { ns.x += dx; }
      if (T) { ns.y -= dy; np.y += dy; }
      if (B) { ns.y += dy; }

      if (ns.x < mn.x) {
        if (L) np.x = m_drag.startPos.x + m_drag.startSize.x - mn.x;
        ns.x = mn.x;
      }
      if (ns.y < mn.y) {
        if (T) np.y = m_drag.startPos.y + m_drag.startSize.y - mn.y;
        ns.y = mn.y;
      }
      root->pos = np;
      root->size = ns;
      return;
    }

    case DragMode::Splitter: {
      DockNode *n = FindNodeById(m_drag.nodeId);
      if (!n || n->IsLeaf() || !down) {
        m_drag = {};
        return;
      }
      DockNode *a = n->child[0].get();
      DockNode *b = n->child[1].get();
      const bool horiz = n->axis == DockAxis::Horizontal;
      m_platform->SetCursor(horiz ? CursorShape::ResizeHorizontal : CursorShape::ResizeVertical);

      const vec2 minA = MinSize(a), minB = MinSize(b);
      const float mA = horiz ? minA.x : minA.y;
      const float mB = horiz ? minB.x : minB.y;
      const float avail = std::max(1.0f, (horiz ? n->size.x : n->size.y) - kSplitter);
      const float delta = horiz ? mouse.x - m_drag.startMouse.x : mouse.y - m_drag.startMouse.y;
      const float startA = horiz ? m_drag.startSize.x : m_drag.startSize.y;

      // clamp to what layout can actually produce, so the splitter never detaches from the mouse
      const float sizeA = std::clamp(startA + delta, mA, std::max(mA, avail - mB));
      n->ratio = sizeA / avail;
      const bool cA = ContainsCentral(a), cB = ContainsCentral(b);
      if (cA != cB)
        n->fixedPx = cA ? avail - sizeA : sizeA;
      return;
    }
  }
}

// ---------------------------------------------------------------------------
// init / frame / window
// ---------------------------------------------------------------------------
void EuclaseGUI::Init(std::shared_ptr<Platform> platform, std::shared_ptr<GraphicsRenderer> renderer,
                      std::string fontPath, uint32_t pixelHeight, int dockableArea) {
  m_platform = std::move(platform);
  m_renderer = std::move(renderer);
  m_device = m_renderer->GetDevice();
  m_dockableArea = dockableArea;
  m_fontPixelHeight = static_cast<float>(pixelHeight);

  m_mainRoot = MakeNode();
  m_mainRoot->central = true;
  m_floatingRoots.clear();

  GraphicsPipelineDesc boxDesc;

  boxDesc.colorFormat = m_renderer->GetSwapchainFormat();
  boxDesc.vertexShader = util::ReadFile("shaders/box.vert");
  boxDesc.fragmentShader = util::ReadFile("shaders/box.frag");
  boxDesc.depthTest = false;
  boxDesc.depthWrite = false;
  boxDesc.blending = true;

  boxDesc.constants = {ShaderConstant(ShaderStage::Vertex, 0, nullptr, sizeof(Box))};

  boxPipeline = m_device->CreatePipeline(boxDesc);

  font = std::make_unique<Font>();
  font->Init(m_device, std::move(fontPath), pixelHeight);

  textDesc.colorFormat = m_renderer->GetSwapchainFormat();
  textDesc.vertexShader = util::ReadFile("shaders/text.vert");
  textDesc.fragmentShader = util::ReadFile("shaders/text.frag");
  textDesc.depthTest = false;
  textDesc.depthWrite = false;
  textDesc.blending = true;

  FontAtlas = ShaderResource(0, ResourceType::CombinedImageSampler, ShaderStage::Fragment, nullptr, 0,
                             nullptr, font->GetTexture());

  TextData = ShaderResource(1, ResourceType::StorageBuffer, ShaderStage::Vertex, nullptr,
                            sizeof(GlyphConstant) * 1000, nullptr);
  PipelineTextBufferSize = 1000;
  textDesc.resources = {FontAtlas, TextData};

  textPipeline = m_device->CreatePipeline(textDesc);

  EventManager::RegisterEvent(EventType::MouseButton, [](Event event) {
    MouseButtonCallback(static_cast<int>(event.data32[0]), static_cast<int>(event.data32[1]), 0);
  });

  EventManager::RegisterEvent(EventType::MouseMove, [](Event event) {
    MouseMoveCallback(event.dataDouble[0], event.dataDouble[1]);
  });

  EventManager::RegisterEvent(EventType::MouseScroll, [](Event event) {
    MouseScrollCallback(event.dataDouble[0], event.dataDouble[1]);
  });

  EventManager::RegisterEvent(EventType::KeyPress, [](Event event) {
    KeyCallback(static_cast<int>(event.data32[0]), static_cast<int>(event.data32[1]),
                static_cast<int>(event.data[0]), 0);
  });
}

void EuclaseGUI::Shutdown() {
  m_TextBatches.clear();
  m_ChromeBatches.clear();
  m_Windows.clear();
  m_WindowRenderData.clear();
  m_floatingRoots.clear();
  m_mainRoot.reset();
  m_overlay.clear();

  font = nullptr;
  textPipeline = nullptr;
  boxPipeline = nullptr;

  font = nullptr;
  FontAtlas = {};
  TextData = {};

  Buffer = nullptr;

  m_device = nullptr;
  m_renderer = nullptr;
  m_platform = nullptr;
}

void EuclaseGUI::BeginFrame() {
  Buffer = m_renderer->GetCommandBuffer();

  auto [w, h] = m_renderer->GetExtent();
  w *= 2;
  h *= 2;
  m_viewport = {static_cast<float>(w), static_cast<float>(h)};

  UpdateHover();   // uses last frame's layout
  HandleInput();   // docking, tab drags, splitters, floating resize
  LayoutAll();     // so Begin() sees this frame's final rects
}

void EuclaseGUI::EndFrame() {
  std::copy(std::begin(mouseButtons), std::end(mouseButtons), std::begin(previousMouseButtons));
  mouseScrollDelta = {0.0, 0.0};
  // Windows that weren't Begin()'d this frame are gone (closed): free their dock slot.
  std::vector<std::string> gone;
  auto collect = [&](DockNode *n) {
    if (!n->IsLeaf())
      return;
    for (const auto &t : n->tabs)
      if (!m_seenThisFrame.count(t))
        gone.push_back(t);
  };
  Visit(m_mainRoot.get(), collect);
  for (auto &r : m_floatingRoots)
    Visit(r.get(), collect);
  for (const auto &t : gone) {
    UndockWindow(t);
    m_Windows.erase(t);
    m_WindowRenderData.erase(t);
  }
  m_seenThisFrame.clear();

  Buffer = nullptr;
}

bool EuclaseGUI::Begin(Window &window) {
  currentWindow = {};
  if (!window.open) {
    currentWindow.hidden = true;
    return false;  // not marked as seen => removed from the dock tree
  }
  m_seenThisFrame.insert(window.title);

  DockNode *leaf = FindLeafWithWindow(window.title);
  if (!leaf) {
    m_Windows[window.title] = window;
    RegisterWindow(window);
    leaf = FindLeafWithWindow(window.title);
  }

  const bool active = leaf->tabs[leaf->active] == window.title;

  // the dock tree owns the geometry
  window.position = leaf->pos;
  window.size = leaf->size;
  m_Windows[window.title] = window;

  currentWindow.window = &window;
  currentWindow.hidden = !active;
  if (!active)
    return false;

  const float barH = BarHeight(leaf);
  currentWindow.position = window.position;
  currentWindow.size = window.size;
  currentWindow.contentSize = {0.0f, 0.0f};

  currentWindow.pointer = {window.position.x + window.borderSize.x + window.padding.x,
                           window.position.y + barH + window.borderSize.y + window.padding.y};

  currentWindow.lineStartY = currentWindow.pointer.y;
  currentWindow.lineEndX = currentWindow.pointer.x;
  currentWindow.lineHeight = 0.0f;
  currentWindow.sameLine = false;
  currentWindow.sameLineSpacing = 0.0f;

  currentWindow.renderData.boxes.push_back(MakeBox(window.position, window.size, window.backgroundColor));
  return true;
}

void EuclaseGUI::End() {
  if (!currentWindow.window)
    return;
  if (currentWindow.hidden) {
    currentWindow = {};
    return;
  }

  constexpr float minWidth = 50.0f;
  constexpr float minHeight = 30.0f;

  // Include the final line in the content size.
  currentWindow.contentSize.x =
      std::max(currentWindow.contentSize.x,
               currentWindow.lineEndX - (currentWindow.position.x + currentWindow.window->borderSize.x));

  currentWindow.contentSize.y =
      std::max(currentWindow.contentSize.y,
               currentWindow.lineStartY + currentWindow.lineHeight -
                   (currentWindow.position.y + currentWindow.window->borderSize.y));

  currentWindow.window->minSize.x = std::max(minWidth, currentWindow.contentSize.x);
  currentWindow.window->minSize.y = std::max(minHeight, currentWindow.contentSize.y);

  m_WindowRenderData[currentWindow.window->title] = std::move(currentWindow.renderData);
  currentWindow = {};
}

void EuclaseGUI::SameLine(float spacing) {
  currentWindow.sameLine = true;
  currentWindow.sameLineSpacing = spacing;
}

// ---------------------------------------------------------------------------
// rendering
// ---------------------------------------------------------------------------

// drawText() writes into currentWindow, so borrow it with a scratch window.
void EuclaseGUI::DrawTextAt(WindowRenderData &out, std::string_view text, vec2 pos, float size, vec4 color) {
  CurrentWindow saved = std::move(currentWindow);
  currentWindow = {};
  currentWindow.window = &m_chromeWindow;
  currentWindow.position = pos;
  currentWindow.pointer = pos;
  currentWindow.lineStartY = pos.y;
  currentWindow.lineEndX = pos.x;

  drawText(text, size, color);

  out.texts.insert(out.texts.end(), currentWindow.renderData.texts.begin(),
                   currentWindow.renderData.texts.end());
  currentWindow = std::move(saved);
}

// Tab bars, splitters and the border of floating trees.
WindowRenderData EuclaseGUI::BuildChrome(DockNode &root) {
  WindowRenderData rd;

  Visit(&root, [&](DockNode *n) {
    if (!n->IsLeaf()) {
      vec2 sp, ss;
      SplitterRect(n, sp, ss);
      const bool hot = m_hover.splitter == n || (m_drag.mode == DragMode::Splitter && m_drag.nodeId == n->id);
      rd.boxes.push_back(MakeBox(sp, ss, hot ? vec4{0.35f, 0.45f, 0.9f, 1.0f} : vec4{0.08f, 0.08f, 0.08f, 1.0f}));
      return;
    }
    if (n->tabs.empty())
      return;
    const float barH = BarHeight(n);
    if (barH <= 0.0f)
      return;

    const int count = static_cast<int>(n->tabs.size());
    const int activeIdx = std::clamp(n->active, 0, count - 1);
    rd.boxes.push_back(MakeBox(n->pos, {n->size.x, barH}, W(n->tabs[activeIdx]).HeaderColor));

    const float tabW = TabWidth(n);
    for (int i = 0; i < count; ++i) {
      const Window &w = W(n->tabs[i]);
      const vec2 tp = {n->pos.x + tabW * static_cast<float>(i), n->pos.y};
      const bool active = i == activeIdx;
      const bool hovered = m_drag.mode == DragMode::None && m_hover.leaf == n && m_hover.tabIndex == i;

      if (count > 1) {
        const vec4 tabColor = active ? vec4{0.26f, 0.26f, 0.30f, 1.0f}
                                     : (hovered ? vec4{0.20f, 0.20f, 0.23f, 1.0f} : vec4{0.13f, 0.13f, 0.15f, 1.0f});
        rd.boxes.push_back(MakeBox({tp.x + 1.0f, tp.y + 2.0f}, {tabW - 2.0f, barH - 2.0f}, tabColor));
        if (active)
          rd.boxes.push_back(MakeBox({tp.x + 1.0f, tp.y + barH - 3.0f}, {tabW - 2.0f, 3.0f},
                                     vec4{0.3f, 0.5f, 1.0f, 1.0f}));
      }

      // no font metrics here, so truncate by an approximate advance width
      const float adv = std::max(1.0f, m_fontPixelHeight * 0.55f);
      const size_t maxChars = static_cast<size_t>(std::max(0.0f, (tabW - 20.0f) / adv));
      std::string label = w.title;
      if (label.size() > maxChars) {
        if (maxChars >= 3) {
          label.resize(maxChars - 2);
          label += "..";
        } else {
          label.resize(maxChars);
        }
      }
      DrawTextAt(rd, label, {tp.x + 10.0f, tp.y + (barH - m_fontPixelHeight) * 0.5f}, 1.0f,
                 active ? vec4{1.0f, 1.0f, 1.0f, 1.0f} : vec4{0.7f, 0.7f, 0.7f, 1.0f});
    }
  });

  if (&root != m_mainRoot.get()) {  // floating tree outline
    const vec4 c{0.05f, 0.05f, 0.05f, 1.0f};
    const float t = 2.0f;
    rd.boxes.push_back(MakeBox(root.pos, {root.size.x, t}, c));
    rd.boxes.push_back(MakeBox({root.pos.x, root.pos.y + root.size.y - t}, {root.size.x, t}, c));
    rd.boxes.push_back(MakeBox(root.pos, {t, root.size.y}, c));
    rd.boxes.push_back(MakeBox({root.pos.x + root.size.x - t, root.pos.y}, {t, root.size.y}, c));
  }
  return rd;
}

void EuclaseGUI::GrowTextPipeline(size_t needed) {
  if (needed <= PipelineTextBufferSize)
    return;
  m_renderer->WaitIdle();
  PipelineTextBufferSize = static_cast<unsigned int>(
      std::max<size_t>(needed + needed / 2, static_cast<size_t>(PipelineTextBufferSize) * 2));
  TextData.size = sizeof(GlyphConstant) * PipelineTextBufferSize;
  textDesc.resources = {FontAtlas, TextData};
  textPipeline = m_device->CreatePipeline(textDesc);
}

void EuclaseGUI::DrawBatch(const WindowRenderData &data, TextBatch &batch) {
  for (const Box &b : data.boxes) {
    Box copy = b;
    DrawBox(copy);
  }
  if (data.texts.empty())
    return;

  const size_t frameCount = m_renderer->GetFrameCount();
  const unsigned int frame = m_renderer->GetFrameIndex();
  const size_t required = std::max<size_t>(data.texts.size(), PipelineTextBufferSize);

  if (batch.buffers.size() != frameCount || batch.capacity < required) {
    if (!batch.buffers.empty())
      m_renderer->WaitIdle();
    batch.buffers.clear();
    batch.capacity = required;
    for (size_t i = 0; i < frameCount; ++i)
      batch.buffers.push_back(m_device->CreateBuffer(sizeof(GlyphConstant) * required,
                                                     BufferUsage::Storage, BufferMemory::CPUToGPU));
  }

  textPipeline->Bind(Buffer);
  Buffer->PushResource(FontAtlas, textPipeline.get());

  batch.buffers[frame]->Write(data.texts.data(), data.texts.size() * sizeof(GlyphConstant));
  TextData.buffer = batch.buffers[frame];
  Buffer->PushResource(TextData, textPipeline.get());
  Buffer->Draw(6, static_cast<unsigned int>(data.texts.size()));
}

void EuclaseGUI::Render() {
  // roots back-to-front: main tree, then floating trees in z-order
  std::vector<DockNode *> roots;
  roots.push_back(m_mainRoot.get());
  for (auto &r : m_floatingRoots)
    roots.push_back(r.get());

  std::vector<WindowRenderData> chrome;
  chrome.reserve(roots.size());
  size_t maxGlyphs = 0;
  for (DockNode *r : roots) {
    chrome.push_back(BuildChrome(*r));
    maxGlyphs = std::max(maxGlyphs, chrome.back().texts.size());
  }
  for (const auto &[title, data] : m_WindowRenderData)
    maxGlyphs = std::max(maxGlyphs, data.texts.size());

  // grow the pipeline's storage range *before* recording any text draw
  GrowTextPipeline(maxGlyphs);

  while (m_ChromeBatches.size() < roots.size())
    m_ChromeBatches.emplace_back();

  for (size_t i = 0; i < roots.size(); ++i) {
    Visit(roots[i], [&](DockNode *n) {
        if (!n->IsLeaf() || n->tabs.empty())
          return;

      const std::string &title =
          n->tabs[std::clamp(n->active, 0, static_cast<int>(n->tabs.size()) - 1)];

      auto it = m_WindowRenderData.find(title);
      if (it != m_WindowRenderData.end())
        DrawBatch(it->second, m_TextBatches[title]);
});
    DrawBatch(chrome[i], m_ChromeBatches[i]);
  }

  for (Box &b : m_overlay)
    DrawBox(b);
  m_overlay.clear();
}

void EuclaseGUI::DrawBox(Box box) {
  boxPipeline->Bind(Buffer);
  Buffer->PushConstant(ShaderStage::Vertex, 0, &box, sizeof(Box), boxPipeline.get());
  Buffer->Draw(6, 1);
}

float EuclaseGUI::PixelsToUnits(float pixels, bool isWidth) {
  auto [width, height] = m_renderer->GetExtent();

  return pixels / static_cast<float>(isWidth ? width : height);
}

vec2 EuclaseGUI::PixelsToUnits(vec2 pixels) {
  return {PixelsToUnits(pixels.x, true), PixelsToUnits(pixels.y, false)};
}

// ---------------------------------------------------------------------------
// platform callbacks
// ---------------------------------------------------------------------------
void EuclaseGUI::MouseButtonCallback(int button, int action, int mods) {
  mouseButtons[button] = action != 0;
}

void EuclaseGUI::MouseMoveCallback(double xpos, double ypos) {
  vec2 newPosition{static_cast<float>(xpos * 2), static_cast<float>(ypos * 2)};
 // newPosition *= 2; // Idk why I have to do this but I guess.
  mouseDelta = newPosition - mousePosition;
  mousePosition = newPosition;
}

void EuclaseGUI::MouseScrollCallback(double xoffset, double yoffset) {
  mouseScroll.x += static_cast<float>(xoffset);
  mouseScroll.y += static_cast<float>(yoffset);
  mouseScrollDelta = {static_cast<float>(xoffset), static_cast<float>(yoffset)};
}

void EuclaseGUI::KeyCallback(int key, int scancode, int action, int mods) {
  if (key < 0)
    return;

  const auto keyCode = static_cast<KeyCode>(key);
  if (keyCode == KeyCode::Unknown)
    return;

  const size_t index = static_cast<size_t>(keyCode);
  if (index >= std::size(keys))
    return;

  keys[index] = action != 0;
}
}  // namespace Euclase