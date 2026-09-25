//
// Created by Loyal on 9/24/26.
//

#include "EuclaseGUI.h"

#include <fstream>
#include <sstream>
#include <string>
#include <utility>
std::string ReadFile(const std::string& path)
{
    std::ifstream file(path);

    if (!file)
        return {};

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

namespace Euclase {
     std::shared_ptr<Platform> EuclaseGUI::platform;
     GraphicsRenderer* EuclaseGUI::renderer;
    std::unique_ptr<GraphicsPipeline> EuclaseGUI::pipeline;
    ShaderConstant EuclaseGUI::constant;
    std::shared_ptr<CommandBuffer> EuclaseGUI::buffer;
    void EuclaseGUI::Init(std::shared_ptr<Platform> platform_, GraphicsRenderer* renderer_) {
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
        constant = Euclase::ShaderConstant{ShaderStage::Vertex, 0, nullptr, sizeof(Box)};

        desc.constants = {constant};

        pipeline = device->CreatePipeline(desc);
    }

    void EuclaseGUI::Shutdown() {
        //constant.buffer = nullptr;
        pipeline = nullptr;
        renderer = nullptr;
        platform = nullptr;
    }

    void EuclaseGUI::BeginFrame() {
        buffer = renderer->GetCommandBuffer();
    }

    void EuclaseGUI::EndFrame() {
        buffer = nullptr;
    }

    void EuclaseGUI::Draw() {

        pipeline->Bind(buffer);
        DrawRectangle(100, 100, 100, 100, 0xFF0000FF);
    }

    void EuclaseGUI::DrawRectangle(uint32_t x, uint32_t y, float width, float height, uint32_t color) {
        auto extent = renderer->GetExtent();
        Box box{
            .location = {(float)x, (float)y},
            .size = {(float)width, (float)height},
            .color = {
                (float)((color >> 24) & 0xFF),
                (float)((color >> 16) & 0xFF),
                (float)((color >> 8) & 0xFF),
                (float)(color & 0xFF)
            },
            .frameSize = {(float)extent.width, (float)extent.height}

        };
        buffer->PushConstant(ShaderStage::Vertex, 0, &box, sizeof(Box), pipeline.get());
        buffer->Draw(6);
    }
} // Euclase