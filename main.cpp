#include <iostream>

#include "platform/Event.h"
#include "platform/Platform.h"
#include "platform/wayland/Platform_Wayland.h"
#include "platform/wayland/WaylandDisplay.h"
#include "platform/wayland/WaylandWindow.h"
#include "Renderer/Vulkan/vulkanContext.h"
#include "Renderer/Vulkan/vulkanRenderer.h"

#include "Renderer/Vulkan/platform/waylandVulkan.h"
int main(int argc, char* argv[])
{
    std::shared_ptr<Euclase::Platform> platform = Euclase::Platform::GetNewWindow();

    if (!platform->CreateWindow(1280, 720, "Euclase")) {
        std::cerr << "Failed to create window\n";
        return 1;
    }

    // VulkanRenderer::Init needs the concrete WaylandDisplay/WaylandWindow
    // (it builds VkWaylandSurfaceCreateInfoKHR from them), not the
    // Platform base interface. If you end up with more than one Platform
    // backend, give Platform a virtual accessor for these instead of
    // downcasting.
    auto* waylandPlatform =
        dynamic_cast<Euclase::Platform_Wayland*>(platform.get());

    if (!waylandPlatform) {
        std::cerr << "Vulkan renderer currently only wired up for the "
                     "Wayland platform\n";
        return 1;
    }

    Euclase::vulkanRenderer renderer;
    std::shared_ptr<Euclase::PlatformVulkan> platformVulkan =
        std::make_shared<Euclase::WaylandVulkan>(Euclase::WaylandVulkan(waylandPlatform->GetDisplay()->GetDisplay(), waylandPlatform->GetWindow()->GetSurface()));

    const bool enableValidation =
#ifdef NDEBUG
        false;
#else
        true;
#endif

    renderer.Init(*platformVulkan, enableValidation);
   // if (!renderer.Init(
   //         *waylandPlatform->GetDisplay(),
    //        *waylandPlatform->GetWindow(),
   //         1280, 720,
    //        enableValidation)) {
//
   //     std::cerr << "Failed to initialize Vulkan renderer\n";
   //     return 1;
   // }

    // Forward compositor-driven resizes (fired from
    // WaylandWindow::Update()) into the renderer's swapchain.
   // Euclase::EventManager::RegisterEvent(
   //     Euclase::EventType::WindowResize,
    //    [&renderer](Euclase::Event event) {
   //         renderer.NotifyResize(event.data32[0], event.data32[1]);
    //    }
   // );

    auto* window = waylandPlatform->GetWindow();

    while (!window->ShouldClose()) {
        platform->Dispatch();  // pumps wl_display + fires WindowResize/WindowClose
        window->Update();      // applies any pending resize, dispatches the event
     //   renderer.DrawFrame();
    }

   // renderer.Shutdown();
    platform->Disconnect();

    return 0;
}
