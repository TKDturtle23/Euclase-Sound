#include <iostream>

#include "platform/Event.h"
#include "platform/Platform.h"
#include "Renderer/RendererFactory.h"

#include "Renderer/Vulkan/vulkanContext.h"
#include "Renderer/Vulkan/vulkanRenderer.h"

#include "Renderer/Vulkan/platform/waylandVulkan.h"
#include <fstream>
#include <sstream>
#include <string>

std::string ReadFile(const std::string& path)
{
    std::ifstream file(path);

    if (!file)
        return {};

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

int main(int argc, char* argv[])
{
    std::shared_ptr<Euclase::Platform> platform = Euclase::Platform::GetNewWindow();

    if (!platform->create(1280, 720, "Euclase")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }



   std::unique_ptr<Euclase::GraphicsRenderer> renderer = Euclase::GraphicsRendererFactory::Create(Euclase::GraphicsAPI::Vulkan);

    const bool enableValidation =
#ifdef NDEBUG
        false;
#else
        true;
#endif

    renderer->Init(platform, enableValidation, 1280, 720);
    auto device = renderer->GetDevice();
    Euclase::GraphicsPipelineDesc desc;
    desc.vertexShader = ReadFile("shaders/triangle.vert");
    desc.fragmentShader = ReadFile("shaders/triangle.frag");
    desc.colorFormat = renderer->GetSwapchainFormat();
    desc.depthFormat = Euclase::TextureFormat::Depth32F;
    desc.depthTest = true;
    desc.blending = true;
    auto pipeline = device->CreatePipeline(desc);


    while (!platform->ShouldClose()) {
        platform->Dispatch();  // pumps wl_display + fires WindowResize/WindowClose
        renderer->BeginFrame();
        auto buffer = renderer->GetCommandBuffer();
        pipeline->Bind(buffer);
        buffer->Draw(3);
        renderer->EndFrame();
    }

   // renderer.Shutdown();
    platform->Disconnect();

    return 0;
}
