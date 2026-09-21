#include "vulkanRenderer.h"

#include "platform/vulkanPlatform.h"

namespace Euclase {

    bool vulkanRenderer::Init(
        PlatformVulkan& platform,
        bool enableValidation)
    {
        const auto extensions =
            platform.GetInstanceExtensions();

        if (!context.Init(
                extensions,
                enableValidation))
        {
            return false;
        }

        try
        {
            surface = std::make_unique<vk::raii::SurfaceKHR>(
                platform.CreateSurface(
                    context.GetInstance()
                )
            );
        }
        catch (const vk::SystemError&)
        {
            context.Destroy();
            return false;
        }

        if (!device.Init(context.GetInstance(), *surface)) {
            context.Destroy();
            return false;
        }

        return true;
    }

    void vulkanRenderer::Destroy()
    {
        device.Destroy();

        surface.reset();

        context.Destroy();
    }

}