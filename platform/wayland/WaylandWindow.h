#pragma once

#include "WaylandBuffer.h"
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-client-protocol.h"

namespace Euclase {

class WaylandDisplay;

class WaylandWindow {
public:
    WaylandWindow() = default;
    ~WaylandWindow();

    WaylandWindow(const WaylandWindow&) = delete;
    WaylandWindow& operator=(const WaylandWindow&) = delete;

    bool Create(WaylandDisplay& display, int width, int height, const char* title);
    void Destroy();

    void Update();
    bool Draw();

    bool ShouldClose() const;

    static void SurfaceConfigure(
        void* data,
        xdg_surface* surface,
        uint32_t serial
    );

    static void ToplevelConfigure(
        void* data,
        xdg_toplevel* toplevel,
        int32_t width,
        int32_t height,
        wl_array* states
    );

    static void ToplevelClose(
        void* data,
        xdg_toplevel* toplevel
    );

    static void ToplevelConfigureBounds(
        void* data,
        xdg_toplevel* toplevel,
        int32_t width,
        int32_t height
    );

    static void ToplevelWmCapabilities(
        void* data,
        xdg_toplevel* toplevel,
        wl_array* capabilities
    );

    static void FrameDone(
        void* data,
        wl_callback* callback,
        uint32_t callbackData
    );
    wl_surface* GetSurface() const;

private:
    bool ResizeBuffer();
    bool RequestFrame();

private:
    WaylandDisplay* display = nullptr;

    wl_surface* surface = nullptr;
    xdg_surface* xdgSurface = nullptr;
    xdg_toplevel* toplevel = nullptr;
    zxdg_toplevel_decoration_v1* decoration = nullptr;

    wl_callback* frameCallback = nullptr;

    //WaylandBuffer buffer;

    int width = 800;
    int height = 600;

    int pendingWidth = 800;
    int pendingHeight = 600;

    bool configured = false;
    bool resizePending = false;

    // True when the compositor is ready for another frame.
    bool frameReady = true;

    // True when something needs to be rendered.
    bool redrawPending = false;

    bool running = true;
};

}