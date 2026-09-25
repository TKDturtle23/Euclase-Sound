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

        pipeline = device->CreatePipeline(desc);
    }

    void EuclaseGUI::Shutdown() {
    }

    void EuclaseGUI::BeginFrame() {
    }

    void EuclaseGUI::EndFrame() {
    }

    void EuclaseGUI::Draw() {
    }

    void EuclaseGUI::DrawRectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    }
} // Euclase