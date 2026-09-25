#include "waylandVulkan.h"

#include <iostream>
#include <vulkan/vulkan_wayland.h>

namespace Euclase {

    WaylandVulkan::WaylandVulkan(
        wl_display* display,
        wl_surface* surface)
        : display(display),
          surface(surface)
    {
    }

    std::vector<const char*>
    WaylandVulkan::GetInstanceExtensions() const
    {
        return {
            VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
        };
    }

    vk::raii::SurfaceKHR
    WaylandVulkan::CreateSurface(
        const vk::raii::Instance& instance
    ) const
    {
        vk::WaylandSurfaceCreateInfoKHR createInfo{
            {},
            display,
            surface
        };

        try
        {
            return vk::raii::SurfaceKHR(
                instance,
                createInfo
            );
        }
        catch (const vk::SystemError& error)
        {
            std::cerr
                << "Failed to create Wayland Vulkan surface: "
                << error.what()
                << '\n';

            throw;
        }
    }

}
