#include "Platform_Wayland.h"

#include "WaylandDisplay.h"
#include "WaylandWindow.h"

#include <iostream>

namespace Euclase {

    Platform_Wayland::~Platform_Wayland()
    {
        Disconnect();
    }

    bool Platform_Wayland::CreateWindow(
        int width,
        int height,
        const char* title)
    {
        display = new WaylandDisplay();

        if (!display->Connect()) {
            delete display;
            display = nullptr;

            return false;
        }

        window = new WaylandWindow();

        if (!window->Create(
            *display,
            width,
            height,
            title
        )) {

            delete window;
            window = nullptr;

            display->Disconnect();

            delete display;
            display = nullptr;

            return false;
        }

        return true;
    }

    void Platform_Wayland::Dispatch()
    {
        if (!display)
            return;

        display->Dispatch();

        if (window)
            window->Update();
    }

    void Platform_Wayland::Disconnect()
    {
        if (window) {
            delete window;
            window = nullptr;
        }

        if (display) {
            display->Disconnect();

            delete display;
            display = nullptr;
        }
    }

    WaylandDisplay * Platform_Wayland::GetDisplay() const {
        return display;
    }
    WaylandWindow * Platform_Wayland::GetWindow() const {
        return window;
    }


}
