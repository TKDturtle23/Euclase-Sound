#include "WaylandDisplay.h"

#include <cstring>
#include <iostream>
#include <poll.h>
#include <xkbcommon/xkbcommon.h>
#include "../Event.h"

namespace Euclase {

namespace {

const xdg_wm_base_listener wmBaseListener = {
    .ping = WaylandDisplay::WmBasePing
};

const wl_registry_listener registryListener = {
    .global = WaylandDisplay::RegistryGlobal,
    .global_remove = WaylandDisplay::RegistryGlobalRemove
};
    const wl_seat_listener seatListener = {
        .capabilities = WaylandDisplay::SeatCapabilities,
        .name = WaylandDisplay::SeatName
    };
}
    void WaylandDisplay::SeatCapabilities(
        void* data,
        wl_seat* seat,
        uint32_t capabilities)
{
    std::cout << "Seat capabilities: "
              << capabilities
              << std::endl;
}

    void WaylandDisplay::SeatName(
        void* data,
        wl_seat* seat,
        const char* name)
{
    std::cout << "Seat name: "
              << name
              << std::endl;
}
    wl_seat* WaylandDisplay::GetSeat() const
{
    return seat;
}
WaylandDisplay::~WaylandDisplay()
{
    Disconnect();
}

bool WaylandDisplay::Connect()
{
    display = wl_display_connect(nullptr);

    if (!display) {
        std::cerr << "Failed to connect to Wayland display" << std::endl;
        return false;
    }

    registry = wl_display_get_registry(display);

    if (!registry) {
        std::cerr << "Failed to get Wayland registry" << std::endl;
        Disconnect();
        return false;
    }

    wl_registry_add_listener(
        registry,
        &registryListener,
        this
    );

    /*
     * The roundtrip allows the compositor to send us
     * all currently available globals.
     */
    if (wl_display_roundtrip(display) == -1) {
        std::cerr << "Wayland registry roundtrip failed" << std::endl;
        Disconnect();
        return false;
    }
    wl_seat_add_listener(
        seat,
        &seatListener,
        this
    );
    if (!compositor) {
        std::cerr << "Wayland compositor not available" << std::endl;
        Disconnect();
        return false;
    }

    if (!shm) {
        std::cerr << "Wayland shared memory interface not available"
                  << std::endl;
        Disconnect();
        return false;
    }

    if (!wmBase) {
        std::cerr << "xdg_wm_base not available" << std::endl;
        Disconnect();
        return false;
    }

    xdg_wm_base_add_listener(
        wmBase,
        &wmBaseListener,
        this
    );

    setupInput();

    return true;
}

void WaylandDisplay::Dispatch()
{
    if (!display)
        return;

    while (wl_display_prepare_read(display) != 0) {
        // another thread already has a read pending, or there's
        // pending queued events -- dispatch those first
        if (wl_display_dispatch_pending(display) == -1) {
            std::cerr << "Wayland display dispatch failed" << std::endl;
            return;
        }
    }

    wl_display_flush(display);

    // Non-blocking poll: 0ms timeout, just check if data is available.
    pollfd pfd{ wl_display_get_fd(display), POLLIN, 0 };
    int ret = poll(&pfd, 1, 0);

    if (ret > 0 && (pfd.revents & POLLIN)) {
        wl_display_read_events(display);
    } else {
        wl_display_cancel_read(display);
    }

    if (wl_display_dispatch_pending(display) == -1) {
        std::cerr << "Wayland display dispatch failed" << std::endl;
    }
}

void WaylandDisplay::Disconnect()
{
    if (decorationManager) {
        zxdg_decoration_manager_v1_destroy(decorationManager);
        decorationManager = nullptr;
    }

    if (wmBase) {
        xdg_wm_base_destroy(wmBase);
        wmBase = nullptr;
    }

    if (shm) {
        wl_shm_destroy(shm);
        shm = nullptr;
    }

    if (compositor) {
        wl_compositor_destroy(compositor);
        compositor = nullptr;
    }

    if (registry) {
        wl_registry_destroy(registry);
        registry = nullptr;
    }

    if (display) {
        wl_display_disconnect(display);
        display = nullptr;
    }
}

void WaylandDisplay::RegistryGlobal(
    void* data,
    wl_registry* registry,
    uint32_t name,
    const char* interface,
    uint32_t version)
{
    auto* wayland = static_cast<WaylandDisplay*>(data);

    std::cout
        << "Wayland global: "
        << interface
        << " v"
        << version
        << " name="
        << name
        << std::endl;

    if (strcmp(interface, wl_compositor_interface.name) == 0) {

        const uint32_t bindVersion =
            version > 4 ? 4 : version;

        wayland->compositor =
            static_cast<wl_compositor*>(
                wl_registry_bind(
                    registry,
                    name,
                    &wl_compositor_interface,
                    bindVersion
                )
            );

    } else if (strcmp(interface, wl_shm_interface.name) == 0) {

        wayland->shm =
            static_cast<wl_shm*>(
                wl_registry_bind(
                    registry,
                    name,
                    &wl_shm_interface,
                    1
                )
            );

    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {

        const uint32_t bindVersion =
            version > 6 ? 6 : version;

        wayland->wmBase =
            static_cast<xdg_wm_base*>(
                wl_registry_bind(
                    registry,
                    name,
                    &xdg_wm_base_interface,
                    bindVersion
                )
            );

    } else if (strcmp(interface, wl_seat_interface.name) == 0) {

        const uint32_t bindVersion =
            version > 7 ? 7 : version;

        wayland->seat =
            static_cast<wl_seat*>(
                wl_registry_bind(
                    registry,
                    name,
                    &wl_seat_interface,
                    bindVersion
                )
            );
    } else if (
        strcmp(
            interface,
            zxdg_decoration_manager_v1_interface.name
        ) == 0
    ) {

        wayland->decorationManager =
            static_cast<zxdg_decoration_manager_v1*>(
                wl_registry_bind(
                    registry,
                    name,
                    &zxdg_decoration_manager_v1_interface,
                    1
                )
            );
    }
}

void WaylandDisplay::RegistryGlobalRemove(
    void* data,
    wl_registry* registry,
    uint32_t name)
{
    std::cout
        << "Wayland global removed: "
        << name
        << std::endl;
}

void WaylandDisplay::WmBasePing(
    void* data,
    xdg_wm_base* wmBase,
    uint32_t serial)
{
    xdg_wm_base_pong(wmBase, serial);
}

wl_display* WaylandDisplay::GetDisplay() const
{
    return display;
}

wl_compositor* WaylandDisplay::GetCompositor() const
{
    return compositor;
}

wl_shm* WaylandDisplay::GetShm() const
{
    return shm;
}

xdg_wm_base* WaylandDisplay::GetWmBase() const
{
    return wmBase;
}

zxdg_decoration_manager_v1*
WaylandDisplay::GetDecorationManager() const
{
    return decorationManager;
}

void WaylandDisplay::setupInput() {
    pointer = wl_seat_get_pointer(seat);
    keyboard = wl_seat_get_keyboard(seat);
}

    static void PointerMotion(
    void* data,
    wl_pointer* pointer,
    uint32_t time,
    wl_fixed_t surfaceX,
    wl_fixed_t surfaceY)
{
    Event event;
    event.type = EventType::MouseMove;
    event.dataDouble[0] = wl_fixed_to_double(surfaceX);
    event.dataDouble[1] = wl_fixed_to_double(surfaceY);
    EventManager::DispatchEvent(event);

}

    static void PointerButton(
    void* data,
    wl_pointer* pointer,
    uint32_t serial,
    uint32_t time,
    uint32_t button,
    uint32_t state) {
    Event event;
    if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
        event.type = EventType::MouseButtonDown;
    } else {
        event.type = EventType::MouseButtonUp;
    }
    event.data32[0] = button;
    event.data32[1] = state;
    EventManager::DispatchEvent(event);

}
    static void PointerAxis(
    void* data,
    wl_pointer* pointer,
    uint32_t time,
    uint32_t axis,
    wl_fixed_t value) {
    Event event;
    event.type = EventType::MouseScroll;
    event.dataDouble[0] = wl_fixed_to_double(value);
    EventManager::DispatchEvent(event);
}

}
