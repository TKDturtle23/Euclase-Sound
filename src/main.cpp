#include <algorithm>
#include <iostream>
#include <thread>

#include "GUI/EuclaseGUI.h"
#include "Renderer/GraphicsTypes.h"
#include "Renderer/RendererFactory.h"
#include "Renderer/Vulkan/platform/waylandVulkan.h"
#include "Renderer/Vulkan/vulkanContext.h"
#include "Renderer/Vulkan/vulkanRenderer.h"
#include "platform/Event.h"
#include "platform/Platform.h"

int main(int argc, char* argv[]) {
  std::shared_ptr<Euclase::Platform> platform =
      Euclase::Platform::GetNewWindow();

  if (!platform->create(1280, 720, "Euclase")) {
    std::cerr << "Failed to create window\n";
    return 1;
  }

  std::shared_ptr<Euclase::GraphicsRenderer> renderer =
      Euclase::GraphicsRendererFactory::Create(Euclase::GraphicsAPI::Vulkan);

  const bool enableValidation =
#ifdef NDEBUG
      false;
#else
      true;
#endif

  if (!renderer->Init(platform, enableValidation, 1280, 720)) return 1;
  Euclase::EuclaseGUI::Init(platform, renderer, "assets/fonts/arial/ARIAL.TTF", 32, 100);


    Euclase::Window window;
  window.title = "Test!";
    window.position = {100, 100};
    window.size = {500, 500};

    Euclase::Window window2;
  window2.title = "Test2!";
    window2.position = {200, 200};
    window2.size = {500, 500};



  while (!platform->ShouldClose()) {
    platform->Dispatch();  // pumps wl_display + fires WindowResize/WindowClose
    if (!renderer->BeginFrame()) continue;
    Euclase::EuclaseGUI::BeginFrame();
    Euclase::vec2 p, s;

    if (Euclase::EuclaseGUI::Begin(window)) {
      Euclase::EuclaseGUI::Text("Hello, Euclase!", 2);
      Euclase::EuclaseGUI::SameLine(6);
      if (Euclase::EuclaseGUI::Button("Click me!", {200, 50}, {0.1, 0.1, 0.4, 1.0})) {
        std::cout << "Button clicked!\n";
      }
      Euclase::EuclaseGUI::End();
    }
    if (Euclase::EuclaseGUI::Begin(window2)) {
      Euclase::EuclaseGUI::Text("testing", 2);
      Euclase::EuclaseGUI::End();
    }

    Euclase::EuclaseGUI::Render();
    Euclase::EuclaseGUI::EndFrame();

    renderer->EndFrame();
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
  }

  Euclase::EuclaseGUI::Shutdown();
  renderer->Destroy();
  platform->Disconnect();

  return 0;
}
