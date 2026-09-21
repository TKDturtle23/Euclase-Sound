//
// Created by Loyal on 9/20/26.
//

#include "Platform.h"
#include "../Defines.h"

#ifdef Wayland
#include "wayland/Platform_Wayland.h"
#endif
namespace Euclase {

    std::shared_ptr<Platform> Platform::GetNewWindow() {

#ifdef Wayland
        return std::make_shared<Platform_Wayland>();
#endif
        return nullptr;

    }
} // Euclase