#include "WaylandDisplay.h"

#include <cstring>
#include <iostream>
#include <poll.h>
#include <sys/mman.h>
#include <unistd.h>
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
    const wl_pointer_listener pointerListener = {
        .enter = WaylandDisplay::PointerEnter,
        .leave = WaylandDisplay::PointerLeave,
        .motion = WaylandDisplay::PointerMotion,
        .button = WaylandDisplay::PointerButton,
        .axis = WaylandDisplay::PointerAxis,
        .frame = WaylandDisplay::PointerFrame,
        .axis_source = nullptr,
        .axis_stop = nullptr,
        .axis_discrete = nullptr,
        .axis_value120 = nullptr,
        .axis_relative_direction = nullptr
    };
    const wl_keyboard_listener keyboardListener = {
        .keymap = WaylandDisplay::KeyboardKeymap,
        .enter = WaylandDisplay::KeyboardEnter,
        .leave = WaylandDisplay::KeyboardLeave,
        .key = WaylandDisplay::KeyboardKey,
        .modifiers = WaylandDisplay::KeyboardModifiers,
        .repeat_info = WaylandDisplay::KeyboardRepeatInfo
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

    cursorTheme = wl_cursor_theme_load(nullptr, 24, shm);

    if (!cursorTheme) {
        std::cerr << "Failed to load cursor theme\n";
        Disconnect();
        return false;
    }

    cursorSurface = wl_compositor_create_surface(compositor);

    if (!cursorSurface) {
        std::cerr << "Failed to create cursor surface\n";
        Disconnect();
        return false;
    }
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
    if (xkbState) { xkb_state_unref(xkbState); xkbState = nullptr; }
    if (xkbKeymap) { xkb_keymap_unref(xkbKeymap); xkbKeymap = nullptr; }
    if (xkbContext) { xkb_context_unref(xkbContext); xkbContext = nullptr; }
    if (pointer) { wl_pointer_destroy(pointer); pointer = nullptr; }
    if (keyboard) { wl_keyboard_destroy(keyboard); keyboard = nullptr; }
    if (seat) { wl_seat_destroy(seat); seat = nullptr; }
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

    if (cursorSurface) {
        wl_surface_destroy(cursorSurface);
        cursorSurface = nullptr;
    }

    if (cursorTheme) {
        wl_cursor_theme_destroy(cursorTheme);
        cursorTheme = nullptr;
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
    if (!seat) return;
    pointer = wl_seat_get_pointer(seat);
    keyboard = wl_seat_get_keyboard(seat);
    if (pointer) wl_pointer_add_listener(pointer, &pointerListener, this);
    if (keyboard) wl_keyboard_add_listener(keyboard, &keyboardListener, this);
    xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
}
    void WaylandDisplay::PointerEnter(
        void* data,
        wl_pointer*,
        uint32_t serial,
        wl_surface*,
        wl_fixed_t x,
        wl_fixed_t y)
{
    auto* self = static_cast<WaylandDisplay*>(data);

    self->pointerSerial = serial;

    Event e{};
    e.type = EventType::MouseMove;
    e.dataDouble[0] = wl_fixed_to_double(x);
    e.dataDouble[1] = wl_fixed_to_double(y);

    EventManager::DispatchEvent(e);
}
void WaylandDisplay::PointerLeave(void*, wl_pointer*, uint32_t, wl_surface*) {}
void WaylandDisplay::PointerMotion(
    void* data,
    wl_pointer* pointer,
    uint32_t time,
    wl_fixed_t surfaceX,
    wl_fixed_t surfaceY)
{
    Event event{};
    event.type = EventType::MouseMove;
    event.dataDouble[0] = wl_fixed_to_double(surfaceX);
    event.dataDouble[1] = wl_fixed_to_double(surfaceY);
    EventManager::DispatchEvent(event);

}

void WaylandDisplay::PointerButton(
    void* data,
    wl_pointer* pointer,
    uint32_t serial,
    uint32_t time,
    uint32_t button,
    uint32_t state)
{
    MouseButton mouseButton;

    switch (button) {
        case 272:
            mouseButton = MouseButton::Left;
            break;

        case 273:
            mouseButton = MouseButton::Right;
            break;

        case 274:
            mouseButton = MouseButton::Middle;
            break;

        case 275:
            mouseButton = MouseButton::X1;
            break;

        case 276:
            mouseButton = MouseButton::X2;
            break;

        default:
            return;
    }

    const bool pressed =
        state == WL_POINTER_BUTTON_STATE_PRESSED;

    Event event{};

    event.type = pressed
        ? EventType::MouseButtonDown
        : EventType::MouseButtonUp;

    event.data32[0] =
        static_cast<uint32_t>(mouseButton);

    event.data32[1] =
        pressed ? 1 : 0;

    EventManager::DispatchEvent(event);

    event.type = EventType::MouseButton;

    EventManager::DispatchEvent(event);
}
    void WaylandDisplay::PointerAxis(
        void* data,
        wl_pointer* pointer,
        uint32_t time,
        uint32_t axis,
        wl_fixed_t value)
{
    Event event{};
    event.type = EventType::MouseScroll;

    event.data32[0] = axis;

    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
        event.dataDouble[0] = wl_fixed_to_double(value);
    else if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
        event.dataDouble[1] = wl_fixed_to_double(value);

    EventManager::DispatchEvent(event);
}

void WaylandDisplay::PointerFrame(void*, wl_pointer*) {}

void WaylandDisplay::KeyboardKeymap(void* data, wl_keyboard*, uint32_t format, int32_t fd, uint32_t size) {
    auto* self = static_cast<WaylandDisplay*>(data);
    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 || !self->xkbContext) { close(fd); return; }
    void* mapped = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped != MAP_FAILED) {
        xkb_keymap* keymap = xkb_keymap_new_from_string(self->xkbContext, static_cast<const char*>(mapped), XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
        if (keymap) {
            if (self->xkbState) xkb_state_unref(self->xkbState);
            if (self->xkbKeymap) xkb_keymap_unref(self->xkbKeymap);
            self->xkbKeymap = keymap;
            self->xkbState = xkb_state_new(keymap);
        }
        munmap(mapped, size);
    }
    close(fd);
}
void WaylandDisplay::KeyboardEnter(void*, wl_keyboard*, uint32_t, wl_surface*, wl_array*) {}
void WaylandDisplay::KeyboardLeave(void*, wl_keyboard*, uint32_t, wl_surface*) {}
void WaylandDisplay::KeyboardKey(void* data, wl_keyboard*, uint32_t, uint32_t, uint32_t key, uint32_t state) {
    auto* self = static_cast<WaylandDisplay*>(data);
    Event e{}; e.type = state == WL_KEYBOARD_KEY_STATE_PRESSED ? EventType::KeyDown : EventType::KeyUp;
    const xkb_keysym_t symbol = self->xkbState ? xkb_state_key_get_one_sym(self->xkbState, key + 8) : XKB_KEY_NoSymbol;
    KeyCode code = KeyCode::Unknown;
    switch (symbol) {
        case XKB_KEY_space: code = KeyCode::Space; break;
        case XKB_KEY_Escape: code = KeyCode::Escape; break;
        case XKB_KEY_Return: code = KeyCode::Enter; break;
        case XKB_KEY_Tab: code = KeyCode::Tab; break;
        case XKB_KEY_BackSpace: code = KeyCode::Backspace; break;
        case XKB_KEY_Delete: code = KeyCode::Delete; break;
        case XKB_KEY_Insert: code = KeyCode::Insert; break;
        case XKB_KEY_Left: code = KeyCode::Left; break;
        case XKB_KEY_Right: code = KeyCode::Right; break;
        case XKB_KEY_Up: code = KeyCode::Up; break;
        case XKB_KEY_Down: code = KeyCode::Down; break;
        case XKB_KEY_Home: code = KeyCode::Home; break;
        case XKB_KEY_End: code = KeyCode::End; break;
        case XKB_KEY_Page_Up: code = KeyCode::PageUp; break;
        case XKB_KEY_Page_Down: code = KeyCode::PageDown; break;
        case XKB_KEY_Shift_L: code = KeyCode::LeftShift; break;
        case XKB_KEY_Shift_R: code = KeyCode::RightShift; break;
        case XKB_KEY_Control_L: code = KeyCode::LeftControl; break;
        case XKB_KEY_Control_R: code = KeyCode::RightControl; break;
        case XKB_KEY_Alt_L: code = KeyCode::LeftAlt; break;
        case XKB_KEY_Alt_R: code = KeyCode::RightAlt; break;
        case XKB_KEY_Super_L: code = KeyCode::LeftSuper; break;
        case XKB_KEY_Super_R: code = KeyCode::RightSuper; break;
        default:
            if (symbol >= XKB_KEY_a && symbol <= XKB_KEY_z)
                code = static_cast<KeyCode>(static_cast<uint16_t>(KeyCode::A) + symbol - XKB_KEY_a);
            else if (symbol >= XKB_KEY_0 && symbol <= XKB_KEY_9)
                code = static_cast<KeyCode>(static_cast<uint16_t>(KeyCode::Num0) + symbol - XKB_KEY_0);
            else if (symbol >= XKB_KEY_F1 && symbol <= XKB_KEY_F12)
                code = static_cast<KeyCode>(static_cast<uint16_t>(KeyCode::F1) + symbol - XKB_KEY_F1);
            break;
    }
    e.data32[0] = static_cast<uint32_t>(code); e.data32[1] = key;
    e.data[0] = state == WL_KEYBOARD_KEY_STATE_PRESSED ? 1 : 0;
    e.data64[0] = symbol;
    EventManager::DispatchEvent(e);
    e.type = EventType::KeyPress;
    EventManager::DispatchEvent(e);
}
void WaylandDisplay::KeyboardModifiers(void* data, wl_keyboard*, uint32_t, uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group) {
    auto* self = static_cast<WaylandDisplay*>(data);
    if (self->xkbState) xkb_state_update_mask(self->xkbState, depressed, latched, locked, 0, 0, group);
}
void WaylandDisplay::KeyboardRepeatInfo(void*, wl_keyboard*, int32_t, int32_t) {}
    void WaylandDisplay::SetCursor(const char* name)
{
    if (!cursorTheme || !cursorSurface || !pointer)
        return;

    wl_cursor* cursor =
        wl_cursor_theme_get_cursor(cursorTheme, name);

    if (!cursor || cursor->image_count == 0)
        return;

    wl_cursor_image* image = cursor->images[0];

    wl_surface_attach(
        cursorSurface,
        wl_cursor_image_get_buffer(image),
        0,
        0
    );

    wl_surface_damage(
        cursorSurface,
        0,
        0,
        image->width,
        image->height
    );

    wl_surface_commit(cursorSurface);

    wl_pointer_set_cursor(
        pointer,
        pointerSerial,
        cursorSurface,
        image->hotspot_x,
        image->hotspot_y
    );
}
}
