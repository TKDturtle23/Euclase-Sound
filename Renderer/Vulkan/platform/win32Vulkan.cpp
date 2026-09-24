#include "win32Vulkan.h"

namespace Euclase {

    Win32Vulkan::Win32Vulkan(
        HINSTANCE hInstance,
        HWND hwnd
    )
        : hInstance(hInstance),
          hwnd(hwnd)
    {
    }

    std::vector<const char*> Win32Vulkan::GetInstanceExtensions() const
    {
        return {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
        };
    }

    vk::raii::SurfaceKHR Win32Vulkan::CreateSurface(
        const vk::raii::Instance& instance
    ) const
    {
        vk::Win32SurfaceCreateInfoKHR createInfo{
            {},
            hInstance,
            hwnd
        };

        return instance.createWin32SurfaceKHR(createInfo);
    }

}