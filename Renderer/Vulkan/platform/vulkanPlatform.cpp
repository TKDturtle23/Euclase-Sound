//
// Created by lstuart9 on 9/24/2026.
//
#include "vulkanPlatform.h"
#ifdef Wayland
#include "waylandVulkan.h"
#elif defined(_WIN32)
#include "win32Vulkan.h"
#include "../../../platform/win32/Platform_Win32.h"
#endif
namespace Euclase {


    std::shared_ptr<PlatformVulkan> PlatformVulkan::create(std::shared_ptr<Platform> platform) {
#ifdef Wayland

#elif defined(_WIN32)
        Platform_Win32* win32Platform = static_cast<Platform_Win32*>(platform.get());
return std::make_shared<Win32Vulkan>(GetModuleHandle(nullptr), win32Platform->GetWindow());
#endif
    }
}