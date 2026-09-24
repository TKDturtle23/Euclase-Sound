#include "Platform_Win32.h"

#include <string>

#if defined(_WIN32)
namespace Euclase {

#include <windows.h>

    LRESULT CALLBACK Platform_Win32::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Platform_Win32* self = nullptr;

    if (msg == WM_NCCREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = reinterpret_cast<Platform_Win32*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<Platform_Win32*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    switch (msg) {
        case WM_CLOSE:
        case WM_DESTROY:
            if (self) self->m_running = false;
            PostQuitMessage(0);
            return 0;

        case WM_SIZE: {
            if (self) {
                UINT w = LOWORD(lParam);
                UINT h = HIWORD(lParam);
                // self->OnResize(w, h); // hook in if you have a resize callback
            }
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
#endif

