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
#include "platform/Audio/Platform_Audio.h"
using namespace Euclase;
void test (InstrumentNotes& notes) {}
    int main(int argc, char* argv[]) {
        std::shared_ptr<Platform> platform =
            Platform::GetNewWindow();

        if (!platform->create(1280, 720, "Euclase")) {
            std::cerr << "Failed to create window\n";
            return 1;
        }

        std::shared_ptr<GraphicsRenderer> renderer =
            GraphicsRendererFactory::Create(GraphicsAPI::Vulkan);

        const bool enableValidation =
      #ifdef NDEBUG
            false;
#else
                true;
#endif

        if (!renderer->Init(platform, enableValidation, 1280, 720)) return 1;
        EuclaseGUI::Init(platform, renderer, "assets/fonts/arial/ARIAL.TTF", 32, 100);
        auto AudioDevices = Platform_Audio::EnumerateDevices();
        std::shared_ptr<Platform_Audio> audio;
        std::cout << "Audio devices: " << AudioDevices.size() << std::endl;
        if (AudioDevices.size() > 0) {
            audio = AudioDevices[0];
        }
        audio->Init(48000, 2);
        auto Output = audio->GetAudioDevices(DeviceFilter::Outputs);
        for (auto &device : Output) {
            std::cout << device.description << std::endl;
        }
        auto Input = audio->GetAudioDevices(DeviceFilter::Inputs);



        Window window;
        window.title = "Test!";
        window.position = {100, 100};
        window.size = {500, 500};

        Window window2;
        window2.title = "Test2!";
        window2.position = {200, 200};
        window2.size = {500, 500};

        InstrumentNotes notes;
    int scroll = 0;
    float scrolly = 0;


        while (!platform->ShouldClose()) {
            platform->Dispatch();  // pumps wl_display + fires WindowResize/WindowClose
            if (!renderer->BeginFrame()) continue;
            EuclaseGUI::BeginFrame();
            vec2 p, s;

            if (EuclaseGUI::Begin(window)) {
                EuclaseGUI::Text("Hello world!", 1);
                EuclaseGUI::InstrumentNotes(-57, 30, notes, test, 30, 1000, window.size.y - (window.padding.y * 4) - window.HeaderHeight,
                    scroll, scrolly, Quantization::Quarter, true);

                EuclaseGUI::End();
            }


            EuclaseGUI::Render();
            EuclaseGUI::EndFrame();

            renderer->EndFrame();
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }

        EuclaseGUI::Shutdown();
        renderer->Destroy();
        platform->Disconnect();

        return 0;
    }


