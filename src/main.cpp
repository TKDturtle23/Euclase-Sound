#include <iostream>

#include "platform/Event.h"
#include "platform/Platform.h"
#include "Renderer/RendererFactory.h"

#include "Renderer/Vulkan/vulkanContext.h"
#include "Renderer/Vulkan/vulkanRenderer.h"

#include "Renderer/Vulkan/platform/waylandVulkan.h"

#include <thread>
#include "Renderer/GraphicsTypes.h"
#include "GUI/EuclaseGUI.h"


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
    Box box{
        .location = {0.0f, 0.0f},
        .size = {0.5f, 0.5f},
        .color = {1.0f, 0.0f, 0.0f, 1.0f}
    };
    renderer->Init(platform, enableValidation, 1280, 720);



    while (!platform->ShouldClose()) {
        platform->Dispatch();  // pumps wl_display + fires WindowResize/WindowClose
        if (!renderer->BeginFrame())
            continue;
        auto buffer = renderer->GetCommandBuffer();
        pipeline->Bind(buffer);
        buffer->PushResource(constant, pipeline.get());
        buffer->Draw(6);
        renderer->EndFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }

   // renderer.Shutdown();
    platform->Disconnect();

    return 0;
}
