//
// Created by Loyal on 9/20/26.
//

#ifndef EUCLASESOUND_EVENT_H
#define EUCLASESOUND_EVENT_H
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>
namespace Euclase {
    // Platform independent keyboard values. Backends translate their native
    // key codes into these values before dispatching an Event.
    enum class KeyCode : uint16_t {
        Unknown = 0,
        Space, Apostrophe, Comma, Minus, Period, Slash,
        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
        Semicolon, Equal,
        A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T,
        U, V, W, X, Y, Z,
        LeftBracket, Backslash, RightBracket, GraveAccent,
        Escape, Enter, Tab, Backspace, Insert, Delete,
        Right, Left, Down, Up, PageUp, PageDown, Home, End,
        CapsLock, ScrollLock, NumLock, PrintScreen, Pause,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        LeftShift, RightShift, LeftControl, RightControl,
        LeftAlt, RightAlt, LeftSuper, RightSuper,
        Menu
    };
    enum class EventType {
        WindowClose,
        WindowResize,
        WindowFocus,
        WindowLostFocus,
        WindowMove,
        WindowRender,
        KeyPress, // data[0] is down or up
        KeyDown,
        KeyUp,
        MouseButton, // data[0] is down or up
        MouseButtonDown,
        MouseButtonUp,
        MouseMove,
        MouseScroll
    };
    enum class MouseButton {
        Left,
        Right,
        Middle,
        X1,
        X2
    };
    struct Event {
        EventType type{};
        uint8_t data[12]{};
        uint32_t data32[2]{};
        uint64_t data64[2]{};
        double dataDouble[2]{};

        // Key events: data32[0] is KeyCode, data32[1] is the native keycode,
        // data64[0] is the xkbcommon keysym, and data[0] is the key state.
        // Mouse events use data32[0] for the button/axis and dataDouble for
        // the pointer position or scroll amount.
    };
    class EventManager {
    public:
        static void RegisterEvent(EventType type, std::function<void(Event)> callback);
        static void DispatchEvent(Event event);
    private:
        static std::unordered_map<uint32_t, std::vector<std::function<void(Event)>>> events;
    };

}

#endif //EUCLASESOUND_EVENT_H
