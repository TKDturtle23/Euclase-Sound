#pragma once

#include <wayland-client.h>

#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-client-protocol.h"

namespace Euclase {

    class WaylandDisplay {
    public:
        WaylandDisplay() = default;
        ~WaylandDisplay();

        WaylandDisplay(const WaylandDisplay&) = delete;
        WaylandDisplay& operator=(const WaylandDisplay&) = delete;

        bool Connect();
        void Dispatch();
        void Disconnect();

        wl_display* GetDisplay() const;
        wl_compositor* GetCompositor() const;
        wl_shm* GetShm() const;
        xdg_wm_base* GetWmBase() const;
        zxdg_decoration_manager_v1* GetDecorationManager() const;
        void setupInput();

        // Wayland protocol callbacks
        static void RegistryGlobal(
            void* data,
            wl_registry* registry,
            uint32_t name,
            const char* interface,
            uint32_t version
        );

        static void RegistryGlobalRemove(
            void* data,
            wl_registry* registry,
            uint32_t name
        );

        static void WmBasePing(
            void* data,
            xdg_wm_base* wmBase,
            uint32_t serial
        );
        wl_seat* GetSeat() const;

        static void SeatCapabilities(
            void* data,
            wl_seat* seat,
            uint32_t capabilities
        );

        static void SeatName(
            void* data,
            wl_seat* seat,
            const char* name
        );
    private:
        wl_display* display = nullptr;
        wl_registry* registry = nullptr;

        wl_compositor* compositor = nullptr;
        wl_shm* shm = nullptr;
        xdg_wm_base* wmBase = nullptr;
        wl_seat* seat = nullptr;
        zxdg_decoration_manager_v1* decorationManager = nullptr;

        wl_pointer* pointer;
        wl_keyboard* keyboard;
    };

}