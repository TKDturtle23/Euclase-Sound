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
    ShaderResource EuclaseGUI::constant;
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
        constant = Euclase::ShaderResource{0, Euclase::ResourceType::UniformBuffer, Euclase::ShaderStage::Vertex, nullptr, sizeof(Box)};
        constant.buffer = device->CreateBuffer(sizeof(Box), Euclase::BufferUsage::Uniform, Euclase::BufferMemory::CPUToGPU);
        desc.resources = {constant};

        pipeline = device->CreatePipeline(desc);
    }

    void EuclaseGUI::Shutdown() {
        constant.buffer = nullptr;
        pipeline = nullptr;
        renderer = nullptr;
        platform = nullptr;
    }

    void EuclaseGUI::BeginFrame() {
    }

    void EuclaseGUI::EndFrame() {
    }

    void EuclaseGUI::Draw() {
        auto buffer = renderer->GetCommandBuffer();
        Box box{
            .location = {0.25f, 0.25f},
            .size = {0.25f, 0.25f},
            .color = {1.0f, 0.0f, 0.0f, 1.0f}
        };
        constant.data = &box;
        constant.size = sizeof(Box);
        pipeline->Bind(buffer);
        buffer->PushResource(constant, pipeline.get());
        buffer->Draw(6);
    }

    void EuclaseGUI::DrawRectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    }
} // Euclase