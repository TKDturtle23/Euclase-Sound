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
    std::shared_ptr<CommandBuffer> EuclaseGUI::Buffer;
     std::unique_ptr<GraphicsPipeline> EuclaseGUI::textPipeline;
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
    bool EuclaseGUI::keys[static_cast<size_t>(KeyCode::Menu) + 1];
    ShaderResource EuclaseGUI::FontAtlas;
     ResizeEdge EuclaseGUI::resizeEdge = ResizeEdge::None;
     Window* EuclaseGUI::resizingWindow = nullptr;

     vec2 EuclaseGUI::resizeStartMouse{};
     vec2 EuclaseGUI::resizeStartPosition{};
     vec2 EuclaseGUI::resizeStartSize{};
    typedef struct {
        Window *window;
        vec2 pointer;
        vec2 size;
        vec2 position;
        vec2 contentSize;
    } CurrentWindow;
    typedef struct {
        vec2 location;
        vec2 size;
        vec4 Color;
        vec2 uv;
        vec2 uvSize;
    } GlyphConstant;
    bool EuclaseGUI::IsMousePressed(MouseButton button)
    {
        const size_t index = static_cast<size_t>(button);

        return mouseButtons[index] && !previousMouseButtons[index];
    }
    bool EuclaseGUI::IsMouseReleased(MouseButton button)
    {
        const size_t index = static_cast<size_t>(button);

        return !mouseButtons[index] && previousMouseButtons[index];
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
                default:
                m_platform->SetCursor(CursorShape::Default);
        }
    }
    static CurrentWindow currentWindow;
    void EuclaseGUI::Init(std::shared_ptr<Platform> platform, std::shared_ptr<GraphicsRenderer> renderer, std::string fontPath, uint32_t pixelHeight) {
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
        boxDesc.constants = {ShaderConstant(ShaderStage::Vertex, 0, nullptr, sizeof(Box))};
        boxPipeline = m_device->CreatePipeline(boxDesc);

        font = std::make_unique<Font>();
        font->Init(m_device, std::move(fontPath), pixelHeight);

        GraphicsPipelineDesc textDesc;
        textDesc.colorFormat = m_renderer->GetSwapchainFormat();
        textDesc.vertexShader = util::ReadFile("shaders/text.vert");
        textDesc.fragmentShader = util::ReadFile("shaders/text.frag");
        textDesc.depthTest = false;
        textDesc.depthWrite = false;
        textDesc.blending = true;
        textDesc.constants = {ShaderConstant(ShaderStage::Vertex, 0, nullptr, sizeof(GlyphConstant))};
        FontAtlas = ShaderResource(0, ResourceType::CombinedImageSampler, ShaderStage::Fragment, nullptr, 0, nullptr, font->GetTexture());
        textDesc.resources = {FontAtlas};
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

namespace Euclase {
     std::shared_ptr<Platform> EuclaseGUI::platform;
     std::shared_ptr<GraphicsRenderer> EuclaseGUI::renderer;
    std::unique_ptr<GraphicsPipeline> EuclaseGUI::pipeline;
    void EuclaseGUI::Init(std::shared_ptr<Platform> platform_, std::shared_ptr<GraphicsRenderer> renderer_) {
        platform = std::move(platform_);
        renderer = std::move(renderer_);

        auto device = renderer->GetDevice();
        Euclase::GraphicsPipelineDesc desc;
        desc.vertexShader = ReadFile("shaders/triangle.vert");
        desc.fragmentShader = ReadFile("shaders/triangle.frag");
        desc.colorFormat = renderer->GetSwapchainFormat();
        desc.depthFormat = Euclase::TextureFormat::Depth32F;
        desc.depthTest = true;
        desc.blending = true;
        auto constant = Euclase::ShaderResource{0, Euclase::ResourceType::UniformBuffer, Euclase::ShaderStage::Vertex, nullptr, sizeof(Box)};
        constant.buffer = device->CreateBuffer(sizeof(Box), Euclase::BufferUsage::Uniform, Euclase::BufferMemory::CPUToGPU);
        desc.resources = {constant};

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
    }

    void EuclaseGUI::BeginFrame() {
    }



    // Left mouse button released -> stop resizing
    if (!mouseButtons[0]) {
        resizingWindow = nullptr;
        resizeEdge = ResizeEdge::None;
        return;
    }

    Window& window = *resizingWindow;

    const vec2 mouseDelta = mousePosition - resizeStartMouse;

    vec2 newPosition = resizeStartPosition;
    vec2 newSize = resizeStartSize;

    float minWidth = window.minSize.x;
    float minHeight = window.minSize.y;

    switch (resizeEdge) {
        case ResizeEdge::Left:
            newPosition.x = resizeStartPosition.x + mouseDelta.x;
            newSize.x = resizeStartSize.x - mouseDelta.x;
            break;

        case ResizeEdge::Right:
            newSize.x = resizeStartSize.x + mouseDelta.x;
            break;

        case ResizeEdge::Top:
            newPosition.y = resizeStartPosition.y + mouseDelta.y;
            newSize.y = resizeStartSize.y - mouseDelta.y;
            break;

        case ResizeEdge::Bottom:
            newSize.y = resizeStartSize.y + mouseDelta.y;
            break;

        case ResizeEdge::TopLeft:
            newPosition.x = resizeStartPosition.x + mouseDelta.x;
            newSize.x = resizeStartSize.x - mouseDelta.x;

            newPosition.y = resizeStartPosition.y + mouseDelta.y;
            newSize.y = resizeStartSize.y - mouseDelta.y;
            break;

        case ResizeEdge::TopRight:
            newSize.x = resizeStartSize.x + mouseDelta.x;

            newPosition.y = resizeStartPosition.y + mouseDelta.y;
            newSize.y = resizeStartSize.y - mouseDelta.y;
            break;

        case ResizeEdge::BottomLeft:
            newPosition.x = resizeStartPosition.x + mouseDelta.x;
            newSize.x = resizeStartSize.x - mouseDelta.x;

            newSize.y = resizeStartSize.y + mouseDelta.y;
            break;

        case ResizeEdge::BottomRight:
            newSize.x = resizeStartSize.x + mouseDelta.x;
            newSize.y = resizeStartSize.y + mouseDelta.y;
            break;

        case ResizeEdge::None:
            break;
    }
        currentWindow.contentSize = {
            0.0f,
            0.0f
        };

    // Minimum size
    if (newSize.x < minWidth) {
        if (resizeEdge == ResizeEdge::Left ||
            resizeEdge == ResizeEdge::TopLeft ||
            resizeEdge == ResizeEdge::BottomLeft) {
            newPosition.x = resizeStartPosition.x +
                            resizeStartSize.x -
                            minWidth;
        }

        newSize.x = minWidth;
    }

    if (newSize.y < minHeight) {
        if (resizeEdge == ResizeEdge::Top ||
            resizeEdge == ResizeEdge::TopLeft ||
            resizeEdge == ResizeEdge::TopRight) {
            newPosition.y = resizeStartPosition.y +
                            resizeStartSize.y -
                            minHeight;
        }

        newSize.y = minHeight;
    }

    window.position = newPosition;
    window.size = newSize;
}

    void EuclaseGUI::EndFrame() {
    }

    void EuclaseGUI::Draw() {
    }
    void EuclaseGUI::MouseMoveCallback(
    double xpos,
    double ypos
) {
        const vec2 newPosition{
            static_cast<float>(xpos * 2),
            static_cast<float>(ypos * 2)
        };

        mouseDelta = newPosition - mousePosition;
        mousePosition = newPosition;

    void EuclaseGUI::DrawRectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    }
    void EuclaseGUI::KeyCallback(
    int key,
    int scancode,
    int action,
    int mods
) {
        if (key < 0)
            return;

        const auto keyCode = static_cast<KeyCode>(key);

        if (keyCode == KeyCode::Unknown)
            return;

        const size_t index =
            static_cast<size_t>(keyCode);

        if (index >= std::size(keys))
            return;

        keys[index] = action != 0;
    }
}
