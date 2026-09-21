#pragma once

#include "vulkanContext.h"
#include "vulkanDevice.h"
#include "platform/vulkanPlatform.h"

namespace Euclase {

    class WaylandDisplay;
    class WaylandWindow;

    class vulkanRenderer
    {
    public:
        bool Init(
            PlatformVulkan& platform,
            bool enableValidation
        );

        void Destroy();

    private:
        VulkanContext context;
        std::unique_ptr<vk::raii::SurfaceKHR> surface;
        VulkanDevice device;
    };

}
