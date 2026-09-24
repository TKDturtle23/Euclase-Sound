#pragma once

//#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan_raii.hpp>

#include "vulkanPlatform.h"

#include <windows.h>

namespace Euclase {

    class Win32Vulkan final : public PlatformVulkan
    {
    public:
        Win32Vulkan(
            HINSTANCE hInstance,
            HWND hwnd
        );

        std::vector<const char*> GetInstanceExtensions() const override;

        vk::raii::SurfaceKHR CreateSurface(
            const vk::raii::Instance& instance
        ) const override;

    private:
        HINSTANCE hInstance;
        HWND hwnd;
    };

}