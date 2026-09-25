//
// Created by lstuart9 on 9/24/2026.
//
#include "vulkanPlatform.h"

#include "../../../platform/wayland/WaylandDisplay.h"
#include "../../../platform/wayland/WaylandWindow.h"
#ifdef Wayland
#include "waylandVulkan.h"
#include "../../../platform/wayland/Platform_Wayland.h"
#elif defined(_WIN32)
#include "win32Vulkan.h"
#include "../../../platform/win32/Platform_Win32.h"
#endif
namespace Euclase {


    std::shared_ptr<PlatformVulkan> PlatformVulkan::create(std::shared_ptr<Platform> platform) {
#ifdef Wayland
        Platform_Wayland* waylandPlatform = static_cast<Platform_Wayland*>(platform.get());
        return std::make_shared<WaylandVulkan>(waylandPlatform->GetDisplay()->GetDisplay(), waylandPlatform->GetWindow()->GetSurface());
#elif defined(_WIN32)
        Platform_Win32* win32Platform = static_cast<Platform_Win32*>(platform.get());
return std::make_shared<Win32Vulkan>(GetModuleHandle(nullptr), win32Platform->GetWindow());
#endif
    }
}