#pragma once

#include "../Platform.h"
#include <Windows.h>
namespace Euclase {



    class Platform_Win32 : public Platform {
    public:
        Platform_Win32() = default;


        ~Platform_Win32() override;

        bool create(
            int width,
            int height,
            const char* title
        ) override;

        void Dispatch() override;

        void Disconnect() override;

        bool ShouldClose() override;

        void GetWindowSize(int &outWidth, int &outHeight) const;

        [[nodiscard]] HWND GetWindow() const;
    private:
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        HWND window = nullptr;
        HMODULE m_hinstance;
        bool m_running = false;
    };

}