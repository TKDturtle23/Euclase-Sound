//
// Created by Loyal on 9/20/26.
//

#include "Platform.h"
#include "../Defines.h"


#ifdef Wayland
#include "wayland/Platform_Wayland.h"
#elif defined(_WIN32)
#include "win32/Platform_Win32.h"
#endif
namespace Euclase {

    std::shared_ptr<Platform> Platform::GetNewWindow() {

#ifdef Wayland
        return std::make_shared<Platform_Wayland>();
#elif defined(_WIN32)
        return std::make_shared<Platform_Win32>();
#endif
        return nullptr;

    }
} // Euclase