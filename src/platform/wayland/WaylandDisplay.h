#pragma once

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-client-protocol.h"

#include <wayland-cursor.h>
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

        static void PointerEnter(void*, wl_pointer*, uint32_t, wl_surface*, wl_fixed_t, wl_fixed_t);
        static void PointerLeave(void*, wl_pointer*, uint32_t, wl_surface*);
        static void PointerMotion(void*, wl_pointer*, uint32_t, wl_fixed_t, wl_fixed_t);
        static void PointerButton(void*, wl_pointer*, uint32_t, uint32_t, uint32_t, uint32_t);
        static void PointerAxis(void*, wl_pointer*, uint32_t, uint32_t, wl_fixed_t);
        static void PointerFrame(void*, wl_pointer*);

        static void KeyboardKeymap(void*, wl_keyboard*, uint32_t, int32_t, uint32_t);
        static void KeyboardEnter(void*, wl_keyboard*, uint32_t, wl_surface*, wl_array*);
        static void KeyboardLeave(void*, wl_keyboard*, uint32_t, wl_surface*);
        static void KeyboardKey(void*, wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t);
        static void KeyboardModifiers(void*, wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
        static void KeyboardRepeatInfo(void*, wl_keyboard*, int32_t, int32_t);

        void SetCursor(const char* name);
    private:
        wl_display* display = nullptr;
        wl_registry* registry = nullptr;

        wl_cursor_theme* cursorTheme = nullptr;
        wl_surface* cursorSurface = nullptr;

        uint32_t pointerSerial = 0;

        wl_compositor* compositor = nullptr;
        wl_shm* shm = nullptr;
        xdg_wm_base* wmBase = nullptr;
        wl_seat* seat = nullptr;
        zxdg_decoration_manager_v1* decorationManager = nullptr;

        wl_pointer* pointer = nullptr;
        wl_keyboard* keyboard = nullptr;
        xkb_context* xkbContext = nullptr;
        xkb_keymap* xkbKeymap = nullptr;
        xkb_state* xkbState = nullptr;
    };

}
