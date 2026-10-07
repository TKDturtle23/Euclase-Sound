//
// Created by Loyal on 10/6/26.
//
#include "../../../Defines.h"
#include "Platform_Audio.h"
#ifdef Wayland
#include "PipeWire/Platform_Pipewire.h"
#endif

std::vector<std::shared_ptr<Euclase::Platform_Audio>> Euclase::Platform_Audio::EnumerateDevices() {
#ifdef Wayland
    return {std::make_shared<Platform_Pipewire>()};
#endif
}
