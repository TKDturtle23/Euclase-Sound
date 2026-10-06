#include "Platform_Win32.h"

#include <string>

#if defined(_WIN32)
namespace Euclase {

#include <windows.h>

LRESULT CALLBACK Platform_Win32::WindowProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    Platform_Win32* self = nullptr;

    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);

        self = reinterpret_cast<Platform_Win32*>(cs->lpCreateParams);

        SetWindowLongPtr(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(self)
        );
    } else {
        self = reinterpret_cast<Platform_Win32*>(
            GetWindowLongPtr(hwnd, GWLP_USERDATA)
        );
    }

    switch (msg) {

        case WM_CLOSE:
            if (self) {
                Event event{};
                event.type = EventType::WindowClose;
                EventManager::DispatchEvent(event);

                self->m_running = false;
            }

            DestroyWindow(hwnd);
            return 0;


        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;


        case WM_SIZE:
        {
            if (self) {
                const uint32_t width  = LOWORD(lParam);
                const uint32_t height = HIWORD(lParam);

                Event event{};
                event.type = EventType::WindowResize;

                event.data32[0] = width;
                event.data32[1] = height;

                EventManager::DispatchEvent(event);
            }

            return 0;
        }


        // ------------------------------------------------------------
        // Keyboard
        // ------------------------------------------------------------

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            const bool pressed =
                msg == WM_KEYDOWN ||
                msg == WM_SYSKEYDOWN;

            Event event{};

            event.type = pressed
                ? EventType::KeyDown
                : EventType::KeyUp;

            const KeyCode key = Win32KeyToKeyCode(wParam, lParam);

            event.data32[0] =
                static_cast<uint32_t>(key);

            // Win32 virtual-key code.
            event.data32[1] =
                static_cast<uint32_t>(wParam);

            // Native Win32 scan code.
            event.data64[0] =
                static_cast<uint64_t>((lParam >> 16) & 0xff);

            event.data[0] = pressed ? 1 : 0;

            EventManager::DispatchEvent(event);

            // Match Wayland's additional KeyPress event.
            event.type = EventType::KeyPress;

            EventManager::DispatchEvent(event);

            return 0;
        }


        // ------------------------------------------------------------
        // Mouse movement
        // ------------------------------------------------------------

        case WM_MOUSEMOVE:
        {
            Event event{};

            event.type = EventType::MouseMove;

            // GET_X_LPARAM / GET_Y_LPARAM correctly handle
            // signed client coordinates.
            event.dataDouble[0] =
                static_cast<double>(
                    static_cast<short>(LOWORD(lParam))
                );

            event.dataDouble[1] =
                static_cast<double>(
                    static_cast<short>(HIWORD(lParam))
                );

            EventManager::DispatchEvent(event);

            return 0;
        }


        // ------------------------------------------------------------
        // Mouse buttons
        // ------------------------------------------------------------

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        {
            MouseButton button;

            switch (msg) {
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                    button = MouseButton::Left;
                    break;

                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                    button = MouseButton::Right;
                    break;

                default:
                    button = MouseButton::Middle;
                    break;
            }

            const bool pressed =
                msg == WM_LBUTTONDOWN ||
                msg == WM_RBUTTONDOWN ||
                msg == WM_MBUTTONDOWN;

            Event event{};

            event.type = pressed
                ? EventType::MouseButtonDown
                : EventType::MouseButtonUp;

            event.data32[0] =
                static_cast<uint32_t>(button);

            event.data32[1] =
                pressed ? 1u : 0u;

            EventManager::DispatchEvent(event);

            event.type = EventType::MouseButton;

            EventManager::DispatchEvent(event);

            return 0;
        }


        // ------------------------------------------------------------
        // X1 / X2 mouse buttons
        // ------------------------------------------------------------

        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        {
            const WORD xButton = HIWORD(wParam);

            MouseButton button;

            if (xButton == XBUTTON1)
                button = MouseButton::X1;
            else if (xButton == XBUTTON2)
                button = MouseButton::X2;
            else
                return 0;

            const bool pressed =
                msg == WM_XBUTTONDOWN;

            Event event{};

            event.type = pressed
                ? EventType::MouseButtonDown
                : EventType::MouseButtonUp;

            event.data32[0] =
                static_cast<uint32_t>(button);

            event.data32[1] =
                pressed ? 1u : 0u;

            EventManager::DispatchEvent(event);

            event.type = EventType::MouseButton;

            EventManager::DispatchEvent(event);

            // MSDN requires TRUE to indicate that the message
            // was handled for XBUTTON messages.
            return TRUE;
        }


        // ------------------------------------------------------------
        // Mouse wheel
        // ------------------------------------------------------------

        case WM_MOUSEWHEEL:
        {
            const SHORT delta =
                GET_WHEEL_DELTA_WPARAM(wParam);

            Event event{};

            event.type = EventType::MouseScroll;

            event.data32[0] =
                0; // vertical

            event.dataDouble[1] =
                static_cast<double>(delta) /
                static_cast<double>(WHEEL_DELTA);

            EventManager::DispatchEvent(event);

            return 0;
        }


        case WM_MOUSEHWHEEL:
        {
            const SHORT delta =
                GET_WHEEL_DELTA_WPARAM(wParam);

            Event event{};

            event.type = EventType::MouseScroll;

            event.data32[0] =
                1; // horizontal

            event.dataDouble[0] =
                static_cast<double>(delta) /
                static_cast<double>(WHEEL_DELTA);

            EventManager::DispatchEvent(event);

            return 0;
        }


        // ------------------------------------------------------------
        // Focus
        // ------------------------------------------------------------

        case WM_SETFOCUS:
        {
            Event event{};
            event.type = EventType::WindowFocus;

            EventManager::DispatchEvent(event);

            return 0;
        }


        case WM_KILLFOCUS:
        {
            Event event{};
            event.type = EventType::WindowLostFocus;

            EventManager::DispatchEvent(event);

            return 0;
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
Platform_Win32::~Platform_Win32() {
    Disconnect();
}

bool Platform_Win32::create(
    const int width,
    const int height,
    const char* title
) {
    m_hinstance = GetModuleHandle(nullptr);

    LPCSTR className = "SubstancesWindowClass";

    WNDCLASSEX wc = {};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = &Platform_Win32::WindowProc;
    wc.hInstance     = m_hinstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr; // avoid GDI clearing — Vulkan owns this surface
    wc.lpszClassName = className;

    // Register once; ignore "class already exists" on repeated calls
    static bool classRegistered = false;
    if (!classRegistered) {
        if (!RegisterClassEx(&wc)) {
            return false;
        }
        classRegistered = true;
    }

    // title is char*; widen it for the Unicode API
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, title, -1, nullptr, 0);
    std::wstring wideTitle(wideLen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wideTitle.data(), wideLen);

    RECT rect = { 0, 0, width, height };
    DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&rect, style, FALSE);

    window = CreateWindowEx(
        0,
        className,
        reinterpret_cast<LPCSTR>(wideTitle.c_str()),
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        m_hinstance,
        this // passed through WM_NCCREATE -> GWLP_USERDATA
    );

    if (!window) {
        return false;
    }

    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);

    m_running = true;
    return true;
}

void Platform_Win32::Dispatch() {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            m_running = false;
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Platform_Win32::Disconnect() {
    if (window) {
        DestroyWindow(window);
        window = nullptr;
    }
    m_running = false;
}


}
HWND Euclase::Platform_Win32::GetWindow() const {
    return window;
}

bool Euclase::Platform_Win32::ShouldClose() {
    return !m_running;
}

void Euclase::Platform_Win32::GetWindowSize(int& outWidth, int& outHeight) const {
    RECT rect;
    if (window && GetClientRect(window, &rect)) {
        outWidth  = rect.right - rect.left;
        outHeight = rect.bottom - rect.top;
    } else {
        outWidth = outHeight = 0;
    }

}

void Euclase::Platform_Win32::SetCursor(CursorShape shape) {
    LPCSTR cursor = IDC_ARROW;

    switch (shape) {
        case CursorShape::Default:
            cursor = IDC_ARROW;
            break;

        case CursorShape::Text:
            cursor = IDC_IBEAM;
            break;

        case CursorShape::Pointer:
            cursor = IDC_HAND;
            break;

        case CursorShape::ResizeHorizontal:
            cursor = IDC_SIZEWE;
            break;

        case CursorShape::ResizeVertical:
            cursor = IDC_SIZENS;
            break;

        case CursorShape::ResizeTopLeft:
        case CursorShape::ResizeBottomRight:
            cursor = IDC_SIZENWSE;
            break;

        case CursorShape::ResizeTopRight:
        case CursorShape::ResizeBottomLeft:
            cursor = IDC_SIZENESW;
            break;

        case CursorShape::Move:
            cursor = IDC_SIZEALL;
            break;
    }

    HCURSOR hCursor = LoadCursor(nullptr, cursor);

    if (hCursor) {
        ::SetCursor(hCursor);
    }
}
#endif

