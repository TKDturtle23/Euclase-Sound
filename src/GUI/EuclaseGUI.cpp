#include "EuclaseGUI.h"

#include <algorithm>
#include <utility>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>


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
    }

    std::shared_ptr<Platform> EuclaseGUI::m_platform;
    std::shared_ptr<GraphicsRenderer> EuclaseGUI::m_renderer;
    std::shared_ptr<GraphicsDevice> EuclaseGUI::m_device;

    std::unique_ptr<GraphicsPipeline> EuclaseGUI::boxPipeline;
    std::unique_ptr<GraphicsPipeline> EuclaseGUI::textPipeline;

    std::shared_ptr<CommandBuffer> EuclaseGUI::Buffer;

    std::unique_ptr<Font> EuclaseGUI::font;

    vec2 EuclaseGUI::mousePosition{};
    vec2 EuclaseGUI::mouseDelta{};
    vec2 EuclaseGUI::mouseScroll{};
    std::unordered_map<std::string, Window> EuclaseGUI::m_Windows;
    Window *EuclaseGUI::HoveredWindow = nullptr;
    bool EuclaseGUI::previousMouseButtons[
        static_cast<size_t>(MouseButton::X2) + 1
    ]{};

    bool EuclaseGUI::mouseButtons[
        static_cast<size_t>(MouseButton::X2) + 1
    ]{};

    bool EuclaseGUI::keys[
        static_cast<size_t>(KeyCode::Menu) + 1
    ]{};

    ShaderResource EuclaseGUI::FontAtlas;
    ShaderResource EuclaseGUI::TextData;

    ResizeEdge EuclaseGUI::resizeEdge = ResizeEdge::None;
    Window *EuclaseGUI::resizingWindow = nullptr;

    vec2 EuclaseGUI::resizeStartMouse{};
    vec2 EuclaseGUI::resizeStartPosition{};
    vec2 EuclaseGUI::resizeStartSize{};
    int EuclaseGUI::m_dockableArea;
    std::vector<std::string> EuclaseGUI::m_WindowRenderOrder;
    std::unordered_map<std::string, WindowRenderData> EuclaseGUI::m_WindowRenderData;
    unsigned int EuclaseGUI::PipelineTextBufferSize;
    GraphicsPipelineDesc EuclaseGUI::textDesc;
    CurrentWindow EuclaseGUI::currentWindow;

    void EuclaseGUI::BringWindowToFront(const std::string &windowName) // technically back so it renderes last
    {
        auto it = std::find(
            m_WindowRenderOrder.begin(),
            m_WindowRenderOrder.end(),
            windowName
        );

        if (it != m_WindowRenderOrder.end()) {
            std::rotate(
                it,
                it + 1,
                m_WindowRenderOrder.end()
            );
        }
    }

    void EuclaseGUI::AddWindow(const std::string &title) {
        if (std::find(
                m_WindowRenderOrder.begin(),
                m_WindowRenderOrder.end(),
                title
            ) == m_WindowRenderOrder.end()) {
            m_WindowRenderOrder.push_back(title);
        }
    }

    bool EuclaseGUI::IsMousePressed(MouseButton button) {
        const size_t index = static_cast<size_t>(button);

        return mouseButtons[index] &&
               !previousMouseButtons[index];
    }


    bool EuclaseGUI::IsMouseReleased(MouseButton button) {
        const size_t index = static_cast<size_t>(button);

        return !mouseButtons[index] &&
               previousMouseButtons[index];
    }


    ResizeEdge EuclaseGUI::GetResizeEdge(const Window &window) {
        constexpr float resizeBorder = 14.0f;

        const float left = window.position.x;
        const float right = window.position.x + window.size.x;
        const float top = window.position.y;
        const float bottom = window.position.y + window.size.y;
        const float header = window.position.y + window.HeaderHeight;

        const bool onLeft =
                mousePosition.x >= left - resizeBorder &&
                mousePosition.x <= left + resizeBorder;

        const bool onRight =
                mousePosition.x >= right - resizeBorder &&
                mousePosition.x <= right + resizeBorder;

        const bool onTop =
                mousePosition.y >= top - resizeBorder &&
                mousePosition.y <= top + resizeBorder;

        const bool onBottom =
                mousePosition.y >= bottom - resizeBorder &&
                mousePosition.y <= bottom + resizeBorder;
        const bool onHeader =
                mousePosition.y >= window.position.y &&
                mousePosition.y <= header && mousePosition.x >= window.position.x && mousePosition.x <= window.position.
                x + window.size.x;

        if (onLeft && onTop)
            return ResizeEdge::TopLeft;

        if (onRight && onTop)
            return ResizeEdge::TopRight;

        if (onLeft && onBottom)
            return ResizeEdge::BottomLeft;

        if (onRight && onBottom)
            return ResizeEdge::BottomRight;

        if (onLeft)
            return ResizeEdge::Left;

        if (onRight)
            return ResizeEdge::Right;

        if (onTop)
            return ResizeEdge::Top;

        if (onBottom)
            return ResizeEdge::Bottom;
        if (onHeader) {
            return ResizeEdge::Move;
        }

        return ResizeEdge::None;
    }


    void EuclaseGUI::ResizeSetCursor(ResizeEdge edge) {
        switch (edge) {
            case ResizeEdge::TopLeft:
                m_platform->SetCursor(CursorShape::ResizeTopLeft);
                break;

            case ResizeEdge::TopRight:
                m_platform->SetCursor(CursorShape::ResizeTopRight);
                break;

            case ResizeEdge::BottomLeft:
                m_platform->SetCursor(CursorShape::ResizeBottomLeft);
                break;

            case ResizeEdge::BottomRight:
                m_platform->SetCursor(CursorShape::ResizeBottomRight);
                break;

            case ResizeEdge::Left:
                m_platform->SetCursor(CursorShape::ResizeHorizontal);
                break;

            case ResizeEdge::Right:
                m_platform->SetCursor(CursorShape::ResizeHorizontal);
                break;

            case ResizeEdge::Top:
                m_platform->SetCursor(CursorShape::ResizeVertical);
                break;

            case ResizeEdge::Bottom:
                m_platform->SetCursor(CursorShape::ResizeVertical);
                break;
            case ResizeEdge::Move:
                m_platform->SetCursor(CursorShape::Move);
                break;

            default:
                m_platform->SetCursor(CursorShape::Default);
                break;
        }
    }

    std::pair<DockingArea, Window *> EuclaseGUI::GetDockingArea(vec2 mousePosition) {
        for (auto &[title, window]: m_Windows) {
            if (title == resizingWindow->title) {
                continue;
            }
            if (mousePosition.x >= window.position.x && mousePosition.x <= window.position.x + window.size.x &&
                mousePosition.y >= window.position.y && mousePosition.y <= window.position.y + window.size.y) {
                return std::make_pair(DockingArea::Middle, &window);
            }
        }
        int winx, winy;
        m_platform->GetWindowSize(winx, winy);
        winx *= 2;
        winy *= 2;
        // main window
        if (mousePosition.x <= m_dockableArea)
            return std::make_pair(DockingArea::Left, nullptr);
        if (mousePosition.x >= winx - m_dockableArea)
            return std::make_pair(DockingArea::Right, nullptr);
        if (mousePosition.y <= m_dockableArea)
            return std::make_pair(DockingArea::Up, nullptr);
        if (mousePosition.y >= winy - m_dockableArea)
            return std::make_pair(DockingArea::Down, nullptr);

        // middle
        if (std::abs(mousePosition.x - winx / 2) <= m_dockableArea && std::abs(mousePosition.y - winy / 2) <=
            m_dockableArea)
            return std::make_pair(DockingArea::Middle, nullptr);

        return std::make_pair(DockingArea::None, nullptr);
    }

    void EuclaseGUI::Init(
        std::shared_ptr<Platform> platform,
        std::shared_ptr<GraphicsRenderer> renderer,
        std::string fontPath,
        uint32_t pixelHeight, int dockableArea
    ) {
        m_platform = std::move(platform);
        m_renderer = std::move(renderer);
        m_device = m_renderer->GetDevice();
        m_dockableArea = dockableArea;

        GraphicsPipelineDesc boxDesc;

        boxDesc.colorFormat = m_renderer->GetSwapchainFormat();
        boxDesc.vertexShader = util::ReadFile("shaders/box.vert");
        boxDesc.fragmentShader = util::ReadFile("shaders/box.frag");
        boxDesc.depthTest = false;
        boxDesc.depthWrite = false;
        boxDesc.blending = true;

        boxDesc.constants = {
            ShaderConstant(
                ShaderStage::Vertex,
                0,
                nullptr,
                sizeof(Box)
            )
        };

        boxPipeline = m_device->CreatePipeline(boxDesc);


        font = std::make_unique<Font>();
        font->Init(
            m_device,
            std::move(fontPath),
            pixelHeight
        );




        textDesc.colorFormat = m_renderer->GetSwapchainFormat();
        textDesc.vertexShader = util::ReadFile("shaders/text.vert");
        textDesc.fragmentShader = util::ReadFile("shaders/text.frag");
        textDesc.depthTest = false;
        textDesc.depthWrite = false;
        textDesc.blending = true;


        FontAtlas = ShaderResource(
            0,
            ResourceType::CombinedImageSampler,
            ShaderStage::Fragment,
            nullptr,
            0,
            nullptr,
            font->GetTexture()
        );

        TextData = ShaderResource(
            1,
            ResourceType::StorageBuffer,
            ShaderStage::Vertex,
            nullptr,
            sizeof(GlyphConstant) * 1000,
            nullptr
        );
        PipelineTextBufferSize = 1000;
        textDesc.resources = {
            FontAtlas,
            TextData
        };

        textPipeline = m_device->CreatePipeline(textDesc);


        EventManager::RegisterEvent(
            EventType::MouseButton,
            [](Event event) {
                MouseButtonCallback(
                    static_cast<int>(event.data32[0]),
                    static_cast<int>(event.data32[1]),
                    0
                );
            }
        );

        EventManager::RegisterEvent(
            EventType::MouseMove,
            [](Event event) {
                MouseMoveCallback(
                    event.dataDouble[0],
                    event.dataDouble[1]
                );
            }
        );

        EventManager::RegisterEvent(
            EventType::MouseScroll,
            [](Event event) {
                MouseScrollCallback(
                    event.dataDouble[0],
                    event.dataDouble[1]
                );
            }
        );

        EventManager::RegisterEvent(
            EventType::KeyPress,
            [](Event event) {
                KeyCallback(
                    static_cast<int>(event.data32[0]),
                    static_cast<int>(event.data32[1]),
                    static_cast<int>(event.data[0]),
                    0
                );
            }
        );
    }


    void EuclaseGUI::Shutdown() {
        textPipeline = nullptr;
        boxPipeline = nullptr;

        font = nullptr;
        FontAtlas = {};

        Buffer = nullptr;

        m_device = nullptr;
        m_renderer = nullptr;
        m_platform = nullptr;
    }


    void EuclaseGUI::BeginFrame() {
        Buffer = m_renderer->GetCommandBuffer();
        HoveredWindow = nullptr;
        for (auto &[windowName, window]: m_Windows) {
            if (mousePosition.x > window.position.x && mousePosition.x <= window.position.x + window.size.x &&
                mousePosition.y > window.position.y && mousePosition.y <= window.position.y + window.size.y) {
                HoveredWindow = &window;
                break;
            }
        }
        if (!resizingWindow)
            return;

        // Left mouse button released -> stop resizing
        if (!mouseButtons[0]) {
            if (resizeEdge == ResizeEdge::Move) {
                auto docking = GetDockingArea(mousePosition);
                if (docking.first != DockingArea::None) {
                    if (docking.second == nullptr) {
                        // application window
                        int winx, winy;
                        m_platform->GetWindowSize(winx, winy);
                        winx *= 2;
                        winy *= 2;
                        switch (docking.first) {
                            case DockingArea::Left: {
                                resizingWindow->position = {0, 0};
                                resizingWindow->size = {resizingWindow->size.x, static_cast<float>(winy)};
                            }
                            break;
                            case DockingArea::Right: {
                                resizingWindow->position = {winx - resizingWindow->size.x, 0};
                                resizingWindow->size = {resizingWindow->size.x, static_cast<float>(winy)};
                            }
                            break;
                            case DockingArea::Up: {
                                resizingWindow->position = {0, 0};
                                resizingWindow->size = {static_cast<float>(winx), resizingWindow->size.y};
                            }
                            break;
                            case DockingArea::Down: {
                                resizingWindow->position = {0, winy - resizingWindow->size.y};
                                resizingWindow->size = {static_cast<float>(winx), resizingWindow->size.y};
                            }
                            break;
                            case DockingArea::Middle: {
                                resizingWindow->position = {0, 0};
                                resizingWindow->size = {static_cast<float>(winx), static_cast<float>(winy)};
                            }
                            break;
                        }
                    }
                }
            }
            resizingWindow = nullptr;
            resizeEdge = ResizeEdge::None;
            return;
        }


        Window &window = *resizingWindow;

        const vec2 mouseDelta =
                mousePosition - resizeStartMouse;

        vec2 newPosition = resizeStartPosition;
        vec2 newSize = resizeStartSize;

        const float minWidth = window.minSize.x;
        const float minHeight = window.minSize.y;

        switch (resizeEdge) {
            case ResizeEdge::Left:
                newPosition.x =
                        resizeStartPosition.x + mouseDelta.x;

                newSize.x =
                        resizeStartSize.x - mouseDelta.x;
                break;

            case ResizeEdge::Right:
                newSize.x =
                        resizeStartSize.x + mouseDelta.x;
                break;

            case ResizeEdge::Top:
                newPosition.y =
                        resizeStartPosition.y + mouseDelta.y;

                newSize.y =
                        resizeStartSize.y - mouseDelta.y;
                break;

            case ResizeEdge::Bottom:
                newSize.y =
                        resizeStartSize.y + mouseDelta.y;
                break;

            case ResizeEdge::TopLeft:
                newPosition.x =
                        resizeStartPosition.x + mouseDelta.x;

                newSize.x =
                        resizeStartSize.x - mouseDelta.x;

                newPosition.y =
                        resizeStartPosition.y + mouseDelta.y;

                newSize.y =
                        resizeStartSize.y - mouseDelta.y;
                break;

            case ResizeEdge::TopRight:
                newSize.x =
                        resizeStartSize.x + mouseDelta.x;

                newPosition.y =
                        resizeStartPosition.y + mouseDelta.y;

                newSize.y =
                        resizeStartSize.y - mouseDelta.y;
                break;

            case ResizeEdge::BottomLeft:
                newPosition.x =
                        resizeStartPosition.x + mouseDelta.x;

                newSize.x =
                        resizeStartSize.x - mouseDelta.x;

                newSize.y =
                        resizeStartSize.y + mouseDelta.y;
                break;

            case ResizeEdge::BottomRight:
                newSize.x =
                        resizeStartSize.x + mouseDelta.x;

                newSize.y =
                        resizeStartSize.y + mouseDelta.y;
                break;
            case ResizeEdge::Move:
                newPosition.x =
                        resizeStartPosition.x + mouseDelta.x;
                newPosition.y =
                        resizeStartPosition.y + mouseDelta.y;
                break;

            case ResizeEdge::None:
                break;
        }
        if (resizeEdge == ResizeEdge::Move) {
            auto docking = GetDockingArea(mousePosition);
            int winx, winy;
            m_platform->GetWindowSize(winx, winy);
            winx *= 2;
            winy *= 2;
            if (docking.first != DockingArea::None) {
                if (docking.second == nullptr) {
                    // application window

                    vec2 ghostSize;
                    vec2 ghostPosition;
                    switch (docking.first) {
                        case DockingArea::Left: {
                            ghostPosition = {0, 0};
                            ghostSize = {resizingWindow->size.x, static_cast<float>(winy)};
                        }
                        break;
                        case DockingArea::Right: {
                            ghostPosition = {winx - resizingWindow->size.x, 0};
                            ghostSize = {resizingWindow->size.x, static_cast<float>(winy)};
                        }
                        break;
                        case DockingArea::Up: {
                            ghostPosition = {0, 0};
                            ghostSize = {static_cast<float>(winx), resizingWindow->size.y};
                        }
                        break;
                        case DockingArea::Down: {
                            ghostPosition = {0, winy - resizingWindow->size.y};
                            ghostSize = {static_cast<float>(winx), resizingWindow->size.y};
                        }
                        break;
                        case DockingArea::Middle: {
                            ghostPosition = {0, 0};
                            ghostSize = {static_cast<float>(winx), static_cast<float>(winy)};
                        }
                        break;
                    }

                    Box ghost;
                    ghost.size = PixelsToUnits(ghostSize);
                    ghost.location = PixelsToUnits(ghostPosition);
                    ghost.color = {0.1f, 0.1f, 0.5f, 0.5f};
                    DrawBox(ghost);
                }
            } else {
                // spacing for app window
                Box ghost;
                ghost.location = PixelsToUnits({0, 0}); // left
                ghost.size = PixelsToUnits({static_cast<float>(m_dockableArea), static_cast<float>(winy)});
                ghost.color = {0.1f, 0.1f, 0.5f, 0.25f};
                DrawBox(ghost);

                ghost.location = PixelsToUnits({static_cast<float>(winx - m_dockableArea), 0}); // right
                DrawBox(ghost);

                ghost.location = PixelsToUnits({0, 0});
                ghost.size = PixelsToUnits({static_cast<float>(winx), static_cast<float>(m_dockableArea)});
                DrawBox(ghost);

                ghost.location = PixelsToUnits({0, static_cast<float>(winy - m_dockableArea)});
                DrawBox(ghost);

                ghost.size = PixelsToUnits({static_cast<float>(m_dockableArea), static_cast<float>(m_dockableArea)});
                ghost.location = PixelsToUnits({
                    static_cast<float>((winx / 2) - m_dockableArea / 2),
                    static_cast<float>((winy / 2) - m_dockableArea / 2)
                });
                DrawBox(ghost);
            }
        }


        // Minimum width
        if (newSize.x < minWidth) {
            if (resizeEdge == ResizeEdge::Left ||
                resizeEdge == ResizeEdge::TopLeft ||
                resizeEdge == ResizeEdge::BottomLeft) {
                newPosition.x =
                        resizeStartPosition.x +
                        resizeStartSize.x -
                        minWidth;
            }

            newSize.x = minWidth;
        }


        // Minimum height
        if (newSize.y < minHeight) {
            if (resizeEdge == ResizeEdge::Top ||
                resizeEdge == ResizeEdge::TopLeft ||
                resizeEdge == ResizeEdge::TopRight) {
                newPosition.y =
                        resizeStartPosition.y +
                        resizeStartSize.y -
                        minHeight;
            }

            newSize.y = minHeight;
        }


        window.position = newPosition;
        window.size = newSize;
    }


    void EuclaseGUI::EndFrame() {
        std::copy(
            std::begin(mouseButtons),
            std::end(mouseButtons),
            std::begin(previousMouseButtons)
        );

        Buffer = nullptr;
    }


    bool EuclaseGUI::Begin(Window &window) {
        AddWindow(window.title);
        if (window.TextBuffers.empty()) {
            for (int i = 0; i < m_renderer->GetFrameCount(); i++) {
                window.TextBuffers.push_back(m_device->CreateBuffer(sizeof(GlyphConstant) * 1000, BufferUsage::Storage, BufferMemory::CPUToGPU));
            }
        }
        currentWindow.window = &window;
        currentWindow.position = window.position;

        currentWindow.pointer = {
            static_cast<float>(window.position.x + window.borderSize.x + (window.HasHeader ? window.HeaderHeight : 0)),
            static_cast<float>(window.position.y + window.borderSize.y + (window.HasHeader ? window.HeaderHeight : 0))
        };

        currentWindow.size = {
            std::max(window.size.x, window.minSize.x),
            std::max(window.size.y, window.minSize.y)
        };

        window.size = currentWindow.size;

        currentWindow.contentSize = {
            0.0f,
            0.0f
        };

        // Layout state
        currentWindow.lineStartY =
                currentWindow.pointer.y;

        currentWindow.lineEndX =
                currentWindow.pointer.x;

        currentWindow.lineHeight = 0.0f;

        currentWindow.sameLine = false;
        currentWindow.sameLineSpacing = 0.0f;


        // Resizing
        auto edge = GetResizeEdge(window);

        if (HoveredWindow) {
            if (HoveredWindow->title == window.title) {
                ResizeSetCursor(edge);
            }
        }


        if (edge != ResizeEdge::None &&
            resizingWindow == nullptr &&
            mouseButtons[
                static_cast<size_t>(MouseButton::Left)
            ] && HoveredWindow) {
            resizeEdge = edge;

            resizeStartMouse = mousePosition;
            resizeStartPosition = window.position;
            resizeStartSize = window.size;

            resizingWindow = &window;
            BringWindowToFront(window.title);
        }


        Box box;

        box.location =
                PixelsToUnits(currentWindow.position);

        box.size =
                PixelsToUnits(currentWindow.size);

        box.color =
                window.backgroundColor;

        currentWindow.renderData.boxes.push_back(box);


        // header
        if (window.HasHeader) {
            Box header;
            header.color = window.HeaderColor;
            header.location = PixelsToUnits(currentWindow.position);
            header.size = PixelsToUnits({window.size.x, window.HeaderHeight});
            currentWindow.renderData.boxes.push_back(header);
            currentWindow.pointer = {
                static_cast<float>(window.position.x + window.borderSize.x),
                static_cast<float>(window.position.y + window.borderSize.y)
            };
            drawText(window.title, 1.0, {1.0, 1.0, 1.0, 1.0});
        }
        currentWindow.pointer = {
            static_cast<float>(window.position.x + window.borderSize.x + (window.HasHeader ? window.HeaderHeight : 0)),
            static_cast<float>(window.position.y + window.borderSize.y + (window.HasHeader ? window.HeaderHeight : 0))
        };
        m_Windows[window.title] = window;
        return true;
    }


    void EuclaseGUI::End() {
        constexpr float minWidth = 50.0f;
        constexpr float minHeight = 30.0f;

        // Include the final line in the content size.
        currentWindow.contentSize.x =
                std::max(
                    currentWindow.contentSize.x,
                    currentWindow.lineEndX -
                    (
                        currentWindow.position.x +
                        currentWindow.window->borderSize.x
                    )
                );

        currentWindow.contentSize.y =
                std::max(
                    currentWindow.contentSize.y,
                    currentWindow.lineStartY +
                    currentWindow.lineHeight -
                    (
                        currentWindow.position.y +
                        currentWindow.window->borderSize.y
                    )
                );


        currentWindow.window->minSize.x =
                std::max(
                    minWidth,
                    currentWindow.contentSize.x
                );

        currentWindow.window->minSize.y =
                std::max(
                    minHeight,
                    currentWindow.contentSize.y
                );
        m_WindowRenderData[currentWindow.window->title] = currentWindow.renderData;
        currentWindow.renderData = {};
    }

    void EuclaseGUI::Render() {
        unsigned int FrameIndex = m_renderer->GetFrameIndex();
        for (auto window: m_WindowRenderOrder) {
            auto data = m_WindowRenderData[window];
            // boxes
            for (auto box: data.boxes) {
                DrawBox(box);
            }

            textPipeline->Bind(Buffer);

            Buffer->PushResource(
                FontAtlas,
                textPipeline.get()
            );

            if (m_Windows[window].currentBufferGlyphCount < data.texts.size()) {
                m_renderer->WaitIdle();
                for (int i = 0; i < m_renderer->GetFrameCount(); i++) {
                    m_Windows[window].TextBuffers[i] = m_device->CreateBuffer(sizeof(GlyphConstant) * data.texts.size() + 500, BufferUsage::Storage, BufferMemory::CPUToGPU);
                }
                m_Windows[window].currentBufferGlyphCount = data.texts.size();
                if (PipelineTextBufferSize < data.texts.size()) {
                    TextData.size = sizeof(GlyphConstant) * data.texts.size();
                    PipelineTextBufferSize = data.texts.size();
                    textDesc.resources = {
                        FontAtlas,
                        TextData
                    };
                    textPipeline = m_device->CreatePipeline(textDesc);
                }
            }

                m_Windows[window].TextBuffers[FrameIndex]->Write(
                    data.texts.data(),
                    data.texts.size() * sizeof(GlyphConstant)
                );
                TextData.buffer = m_Windows[window].TextBuffers[FrameIndex];

                Buffer->PushResource(
                    TextData,
                    textPipeline.get()
                );

                Buffer->Draw(
                    6,
                    static_cast<unsigned int>(data.texts.size())
                );

        }
    }


    void EuclaseGUI::SameLine(float spacing) {
        currentWindow.sameLine = true;
        currentWindow.sameLineSpacing = spacing;
    }


    void EuclaseGUI::DrawBox(Box &box) {
        boxPipeline->Bind(Buffer);

        Buffer->PushConstant(
            ShaderStage::Vertex,
            0,
            &box,
            sizeof(Box),
            boxPipeline.get()
        );

        Buffer->Draw(6, 1);
    }


    float EuclaseGUI::PixelsToUnits(
        float pixels,
        bool isWidth
    ) {
        auto [width, height] =
                m_renderer->GetExtent();

        return pixels /
               static_cast<float>(
                   isWidth ? width : height
               );
    }


    vec2 EuclaseGUI::PixelsToUnits(vec2 pixels) {
        return {
            PixelsToUnits(pixels.x, true),
            PixelsToUnits(pixels.y, false)
        };
    }


    void EuclaseGUI::MouseButtonCallback(
        int button,
        int action,
        int mods
    ) {
        mouseButtons[button] =
                action != 0;
    }


    void EuclaseGUI::MouseMoveCallback(
        double xpos,
        double ypos
    ) {
        const vec2 newPosition{
            static_cast<float>(xpos * 2),
            static_cast<float>(ypos * 2)
        };

        mouseDelta =
                newPosition - mousePosition;

        mousePosition =
                newPosition;
    }


    void EuclaseGUI::MouseScrollCallback(
        double xoffset,
        double yoffset
    ) {
        mouseScroll.x +=
                static_cast<float>(xoffset);

        mouseScroll.y +=
                static_cast<float>(yoffset);
    }


    void EuclaseGUI::KeyCallback(
        int key,
        int scancode,
        int action,
        int mods
    ) {
        if (key < 0)
            return;

        const auto keyCode =
                static_cast<KeyCode>(key);

        if (keyCode == KeyCode::Unknown)
            return;

        const size_t index =
                static_cast<size_t>(keyCode);

        if (index >= std::size(keys))
            return;

        keys[index] =
                action != 0;
    }
}
