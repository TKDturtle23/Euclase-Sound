#pragma once
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan_raii.hpp>

#include "vulkanPlatform.h"

struct wl_display;
struct wl_surface;

namespace Euclase {

    class WaylandVulkan final : public PlatformVulkan
    {
    public:
        WaylandVulkan(
            wl_display* display,
            wl_surface* surface
        );

        std::vector<const char*> GetInstanceExtensions() const override;

        vk::raii::SurfaceKHR CreateSurface(
            const vk::raii::Instance& instance
        ) const override;

    private:
        wl_display* display;
        wl_surface* surface;
    };

}