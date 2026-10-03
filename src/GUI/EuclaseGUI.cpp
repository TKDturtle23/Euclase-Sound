#include "EuclaseGUI.h"

#include <utility>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>


namespace Euclase {

    namespace util {
        std::string ReadFile(const std::string& path)
        {
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

    ResizeEdge EuclaseGUI::resizeEdge = ResizeEdge::None;
    Window* EuclaseGUI::resizingWindow = nullptr;

    vec2 EuclaseGUI::resizeStartMouse{};
    vec2 EuclaseGUI::resizeStartPosition{};
    vec2 EuclaseGUI::resizeStartSize{};


    typedef struct {
        Window* window;

        vec2 pointer;
        vec2 size;
        vec2 position;
        vec2 contentSize;

        // Layout state
        float lineStartY;
        float lineEndX;
        float lineHeight;

        bool sameLine;
        float sameLineSpacing;
    } CurrentWindow;


    typedef struct {
        vec2 location;
        vec2 size;
        vec4 Color;
        vec2 uv;
        vec2 uvSize;
    } GlyphConstant;


    static CurrentWindow currentWindow;


    bool EuclaseGUI::IsMousePressed(MouseButton button)
    {
        const size_t index = static_cast<size_t>(button);

        return mouseButtons[index] &&
               !previousMouseButtons[index];
    }


    bool EuclaseGUI::IsMouseReleased(MouseButton button)
    {
        const size_t index = static_cast<size_t>(button);

        return !mouseButtons[index] &&
               previousMouseButtons[index];
    }


    ResizeEdge EuclaseGUI::GetResizeEdge(const Window& window)
    {
        constexpr float resizeBorder = 14.0f;

        const float left   = window.position.x;
        const float right  = window.position.x + window.size.x;
        const float top    = window.position.y;
        const float bottom = window.position.y + window.size.y;

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

        return ResizeEdge::None;
    }


    void EuclaseGUI::ResizeSetCursor(ResizeEdge edge)
    {
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

            default:
                m_platform->SetCursor(CursorShape::Default);
                break;
        }
    }


    void EuclaseGUI::Init(
        std::shared_ptr<Platform> platform,
        std::shared_ptr<GraphicsRenderer> renderer,
        std::string fontPath,
        uint32_t pixelHeight
    ) {
        m_platform = std::move(platform);
        m_renderer = std::move(renderer);
        m_device = m_renderer->GetDevice();

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


        GraphicsPipelineDesc textDesc;

        textDesc.colorFormat = m_renderer->GetSwapchainFormat();
        textDesc.vertexShader = util::ReadFile("shaders/text.vert");
        textDesc.fragmentShader = util::ReadFile("shaders/text.frag");
        textDesc.depthTest = false;
        textDesc.depthWrite = false;
        textDesc.blending = true;

        textDesc.constants = {
            ShaderConstant(
                ShaderStage::Vertex,
                0,
                nullptr,
                sizeof(GlyphConstant)
            )
        };

        FontAtlas = ShaderResource(
            0,
            ResourceType::CombinedImageSampler,
            ShaderStage::Fragment,
            nullptr,
            0,
            nullptr,
            font->GetTexture()
        );

        textDesc.resources = {
            FontAtlas
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


    void EuclaseGUI::Shutdown()
    {
        textPipeline = nullptr;
        boxPipeline = nullptr;

        font = nullptr;
        FontAtlas = {};

        Buffer = nullptr;

        m_device = nullptr;
        m_renderer = nullptr;
        m_platform = nullptr;
    }


    void EuclaseGUI::BeginFrame()
    {
        Buffer = m_renderer->GetCommandBuffer();

        if (!resizingWindow)
            return;

        // Left mouse button released -> stop resizing
        if (!mouseButtons[0]) {
            resizingWindow = nullptr;
            resizeEdge = ResizeEdge::None;
            return;
        }

        Window& window = *resizingWindow;

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

            case ResizeEdge::None:
                break;
        }


        // Minimum width
        if (newSize.x < minWidth) {
            if (resizeEdge == ResizeEdge::Left ||
                resizeEdge == ResizeEdge::TopLeft ||
                resizeEdge == ResizeEdge::BottomLeft)
            {
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
                resizeEdge == ResizeEdge::TopRight)
            {
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


    void EuclaseGUI::EndFrame()
    {
        std::copy(
            std::begin(mouseButtons),
            std::end(mouseButtons),
            std::begin(previousMouseButtons)
        );

        Buffer = nullptr;
    }


    void EuclaseGUI::Begin(Window& window)
    {
        currentWindow.window = &window;

        currentWindow.position = window.position;

        currentWindow.pointer = {
            window.position.x + window.borderSize.x,
            window.position.y + window.borderSize.y
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

        ResizeSetCursor(edge);

        if (edge != ResizeEdge::None &&
            resizingWindow == nullptr &&
            mouseButtons[
                static_cast<size_t>(MouseButton::Left)
            ])
        {
            resizeEdge = edge;

            resizeStartMouse = mousePosition;
            resizeStartPosition = window.position;
            resizeStartSize = window.size;

            resizingWindow = &window;
        }


        Box box;

        box.location =
            PixelsToUnits(currentWindow.position);

        box.size =
            PixelsToUnits(currentWindow.size);

        box.color =
            window.backgroundColor;

        DrawBox(box);
    }


    void EuclaseGUI::End()
    {
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
    }


    void EuclaseGUI::SameLine(float spacing)
    {
        currentWindow.sameLine = true;
        currentWindow.sameLineSpacing = spacing;
    }


    void EuclaseGUI::Text(
        std::string_view text,
        float size,
        vec4 color
    ) {
        const float contentStartX =
            currentWindow.position.x +
            currentWindow.window->borderSize.x;

        /*
         * If SameLine() was not called, finish the previous
         * line and start a new one.
         */
        if (!currentWindow.sameLine) {
            currentWindow.lineStartY +=
                currentWindow.lineHeight;

            currentWindow.lineHeight = 0.0f;

            currentWindow.lineEndX =
                contentStartX;
        }

        /*
         * If SameLine() was called, continue from the
         * previous item's right edge.
         */
        currentWindow.pointer = {
            currentWindow.lineEndX +
            (
                currentWindow.sameLine
                    ? currentWindow.sameLineSpacing
                    : 0.0f
            ),

            currentWindow.lineStartY
        };

        currentWindow.sameLine = false;


        const float itemStartX =
            currentWindow.pointer.x;


        textPipeline->Bind(Buffer);

        GlyphConstant glyphConstant;

        Buffer->PushResource(
            FontAtlas,
            textPipeline.get()
        );


        for (auto c : text) {
            auto glyph = font->GetGlyph(c);

            glyphConstant.location = {
                PixelsToUnits(
                    currentWindow.pointer.x +
                    glyph.bearing.x * size,
                    true
                ),

                PixelsToUnits(
                    currentWindow.pointer.y -
                    glyph.bearing.y * size +
                    font->GetFontSize() * size,
                    false
                )
            };

            glyphConstant.size = {
                PixelsToUnits(
                    glyph.size.x * size
                ),

                PixelsToUnits(
                    glyph.size.y * size,
                    false
                )
            };

            glyphConstant.Color = color;
            glyphConstant.uv = glyph.uv;
            glyphConstant.uvSize = glyph.uvSize;


            Buffer->PushConstant(
                ShaderStage::Vertex,
                0,
                &glyphConstant,
                sizeof(GlyphConstant),
                textPipeline.get()
            );

            Buffer->Draw(6);


            currentWindow.pointer.x +=
                glyph.advance * size;
        }


        const float width =
            currentWindow.pointer.x -
            itemStartX;

         float height =
            font->GetFontSize() * size;
        height += 8;


        currentWindow.lineEndX =
            currentWindow.pointer.x;

        currentWindow.lineHeight =
            std::max(
                currentWindow.lineHeight,
                height
            );


        // Update content size immediately.
        currentWindow.contentSize.x =
            std::max(
                currentWindow.contentSize.x,
                currentWindow.lineEndX -
                contentStartX
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
    }


    void EuclaseGUI::drawText(
        std::string_view text,
        float size,
        vec4 color
    ) {
        textPipeline->Bind(Buffer);

        GlyphConstant glyphConstant;

        Buffer->PushResource(
            FontAtlas,
            textPipeline.get()
        );


        for (auto c : text) {
            auto glyph = font->GetGlyph(c);

            glyphConstant.location = {
                PixelsToUnits(
                    currentWindow.pointer.x +
                    glyph.bearing.x * size,
                    true
                ),

                PixelsToUnits(
                    currentWindow.pointer.y -
                    glyph.bearing.y * size +
                    font->GetFontSize() * size,
                    false
                )
            };

            glyphConstant.size = {
                PixelsToUnits(
                    glyph.size.x * size
                ),

                PixelsToUnits(
                    glyph.size.y * size,
                    false
                )
            };

            glyphConstant.Color = color;
            glyphConstant.uv = glyph.uv;
            glyphConstant.uvSize = glyph.uvSize;


            Buffer->PushConstant(
                ShaderStage::Vertex,
                0,
                &glyphConstant,
                sizeof(GlyphConstant),
                textPipeline.get()
            );

            Buffer->Draw(6);


            currentWindow.pointer.x +=
                glyph.advance * size;
        }
    }


    bool EuclaseGUI::Button(
        std::string_view text,
        vec2 size,
        vec4 color
    ) {
        const float contentStartX =
            currentWindow.position.x +
            currentWindow.window->borderSize.x;

        /*
         * Start a new line unless SameLine() was called.
         */
        if (!currentWindow.sameLine) {
            currentWindow.lineStartY +=
                currentWindow.lineHeight;

            currentWindow.lineHeight = 0.0f;

            currentWindow.lineEndX =
                contentStartX;
        }


        currentWindow.pointer = {
            currentWindow.lineEndX +
            (
                currentWindow.sameLine
                    ? currentWindow.sameLineSpacing
                    : 0.0f
            ),

            currentWindow.lineStartY
        };

        currentWindow.sameLine = false;


        const vec2 position =
            currentWindow.pointer;


        const bool hovered =
            mousePosition.x >= position.x &&
            mousePosition.x <= position.x + size.x &&
            mousePosition.y >= position.y &&
            mousePosition.y <= position.y + size.y;


        const bool pressed =
            hovered &&
            mouseButtons[
                static_cast<size_t>(MouseButton::Left)
            ];


        vec4 buttonColor = color;


        if (pressed) {
            buttonColor.x *= 0.7f;
            buttonColor.y *= 0.7f;
            buttonColor.z *= 0.7f;
        }
        else if (hovered) {
            buttonColor.x *= 1.2f;
            buttonColor.y *= 1.2f;
            buttonColor.z *= 1.2f;
        }


        Box box;

        box.location =
            PixelsToUnits(position);

        box.size =
            PixelsToUnits(size);

        box.color =
            buttonColor;

        DrawBox(box);


        // Calculate text dimensions.
        float textWidth = 0.0f;

        for (char c : text) {
            textWidth +=
                font->GetGlyph(c).advance;
        }

        const float textHeight =
            font->GetFontSize();


        // Draw centered text without affecting layout.
        const vec2 oldPointer =
            currentWindow.pointer;

        currentWindow.pointer = {
            position.x +
                (size.x - textWidth) * 0.5f,

            position.y +
                (size.y - textHeight) * 0.5f
        };


        drawText(
            text,
            1.0f,
            {
                1.0f,
                1.0f,
                1.0f,
                1.0f
            }
        );


        currentWindow.pointer =
            oldPointer;


        // Update layout.
        currentWindow.lineEndX =
            position.x + size.x;

        currentWindow.lineHeight =
            std::max(
                currentWindow.lineHeight,
                size.y
            );


        currentWindow.contentSize.x =
            std::max(
                currentWindow.contentSize.x,
                currentWindow.lineEndX -
                contentStartX
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


        return hovered &&
               IsMouseReleased(
                   MouseButton::Left
               );
    }


    void EuclaseGUI::DrawBox(Box& box)
    {
        boxPipeline->Bind(Buffer);

        Buffer->PushConstant(
            ShaderStage::Vertex,
            0,
            &box,
            sizeof(Box),
            boxPipeline.get()
        );

        Buffer->Draw(6);
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


    vec2 EuclaseGUI::PixelsToUnits(vec2 pixels)
    {
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