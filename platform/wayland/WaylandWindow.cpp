#include "WaylandWindow.h"
#include "WaylandDisplay.h"

#include <iostream>
#include <cstdint>

#include "../Event.h"

namespace Euclase {

namespace {

const xdg_surface_listener xdgSurfaceListener = {
    .configure = WaylandWindow::SurfaceConfigure
};

const xdg_toplevel_listener toplevelListener = {
    .configure = WaylandWindow::ToplevelConfigure,
    .close = WaylandWindow::ToplevelClose,
    .configure_bounds = WaylandWindow::ToplevelConfigureBounds,
    .wm_capabilities = WaylandWindow::ToplevelWmCapabilities
};

}

WaylandWindow::~WaylandWindow()
{
    Destroy();
}

bool WaylandWindow::Create(
    WaylandDisplay& waylandDisplay,
    int initialWidth,
    int initialHeight,
    const char* title)
{
    display = &waylandDisplay;

    width = initialWidth;
    height = initialHeight;

    pendingWidth = width;
    pendingHeight = height;

    surface = wl_compositor_create_surface(
        display->GetCompositor()
    );

    if (!surface) {
        std::cerr << "Failed to create Wayland surface"
                  << std::endl;
        return false;
    }

    xdgSurface = xdg_wm_base_get_xdg_surface(
        display->GetWmBase(),
        surface
    );

    if (!xdgSurface) {
        std::cerr << "Failed to create xdg_surface"
                  << std::endl;

        Destroy();
        return false;
    }

    xdg_surface_add_listener(
        xdgSurface,
        &xdgSurfaceListener,
        this
    );

    toplevel = xdg_surface_get_toplevel(xdgSurface);

    if (!toplevel) {
        std::cerr << "Failed to create xdg_toplevel"
                  << std::endl;

        Destroy();
        return false;
    }

    xdg_toplevel_add_listener(
        toplevel,
        &toplevelListener,
        this
    );

    xdg_toplevel_set_title(
        toplevel,
        title ? title : "EuclaseSound"
    );

    /*
     * Ask the compositor for server-side decorations if
     * xdg-decoration is available.
     */
    if (display->GetDecorationManager()) {

        decoration =
            zxdg_decoration_manager_v1_get_toplevel_decoration(
                display->GetDecorationManager(),
                toplevel
            );

        if (decoration) {
            zxdg_toplevel_decoration_v1_set_mode(
                decoration,
                ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE
            );
        }
    }

    /*
     * The first commit has no buffer. This tells the compositor
     * that the xdg_surface is ready to be configured. This part
     * of the handshake is required by the protocol regardless of
     * who ends up rendering into the surface.
     */
    wl_surface_commit(surface);

    /*
     * Wait for the initial xdg_surface.configure.
     */
    while (!configured && running) {

        if (wl_display_dispatch(display->GetDisplay()) == -1) {

            std::cerr
                << "Wayland dispatch failed while configuring window"
                << std::endl;

            running = false;
            break;
        }
    }

    if (!running)
        return false;

    if (pendingWidth > 0)
        width = pendingWidth;

    if (pendingHeight > 0)
        height = pendingHeight;

    resizePending = false;

    /*
     * No SHM buffer to allocate here -- the Vulkan swapchain (created
     * separately, from GetSurface()/width/height) owns presentation
     * from this point on.
     */

    return true;
}

void WaylandWindow::Update()
{
    if (!running)
        return;

    /*
     * Apply a pending compositor resize and let anything downstream
     * (the Vulkan renderer's swapchain) know about it. Vulkan handles
     * its own frame pacing/backpressure via the present mode, so
     * there's nothing else to gate on here.
     */
    if (resizePending) {

        if (pendingWidth > 0)
            width = pendingWidth;

        if (pendingHeight > 0)
            height = pendingHeight;

        resizePending = false;

        Event event{};
        event.type = EventType::WindowResize;
        event.data32[0] = width;
        event.data32[1] = height;
        EventManager::DispatchEvent(event);
    }
}

void WaylandWindow::Destroy()
{
    running = false;

    Event event{};
    event.type = EventType::WindowClose;
    event.data64[0] = reinterpret_cast<uintptr_t>(display);
    EventManager::DispatchEvent(event);

    if (decoration) {

        zxdg_toplevel_decoration_v1_destroy(
            decoration
        );

        decoration = nullptr;
    }

    if (toplevel) {

        xdg_toplevel_destroy(toplevel);
        toplevel = nullptr;
    }

    if (xdgSurface) {

        xdg_surface_destroy(xdgSurface);
        xdgSurface = nullptr;
    }

    if (surface) {

        wl_surface_destroy(surface);
        surface = nullptr;
    }

    display = nullptr;
}

bool WaylandWindow::ShouldClose() const
{
    return !running;
}

wl_surface* WaylandWindow::GetSurface() const
{
    return surface;
}

void WaylandWindow::SurfaceConfigure(
    void* data,
    xdg_surface* surface,
    uint32_t serial)
{
    auto* window =
        static_cast<WaylandWindow*>(data);

    /*
     * ack_configure must happen for every configure -- resize or not --
     * or the compositor will consider the surface unresponsive. This is
     * unaffected by who renders the contents.
     */
    xdg_surface_ack_configure(
        surface,
        serial
    );

    window->configured = true;
}

void WaylandWindow::ToplevelConfigure(
    void* data,
    xdg_toplevel* toplevel,
    int32_t newWidth,
    int32_t newHeight,
    wl_array* states)
{
    auto* window =
        static_cast<WaylandWindow*>(data);

    if (newWidth > 0)
        window->pendingWidth = newWidth;

    if (newHeight > 0)
        window->pendingHeight = newHeight;

    if (newWidth > 0 &&
        newWidth != window->width) {

        window->resizePending = true;
    }

    if (newHeight > 0 &&
        newHeight != window->height) {

        window->resizePending = true;
    }
}

void WaylandWindow::ToplevelClose(
    void* data,
    xdg_toplevel* toplevel)
{
    auto* window =
        static_cast<WaylandWindow*>(data);

    window->running = false;
}

void WaylandWindow::ToplevelConfigureBounds(
    void* data,
    xdg_toplevel* toplevel,
    int32_t width,
    int32_t height)
{
    /*
     * Bounds are not the actual requested window size.
     * They are useful later when implementing size constraints.
     */
}

void WaylandWindow::ToplevelWmCapabilities(
    void* data,
    xdg_toplevel* toplevel,
    wl_array* capabilities)
{
    /*
     * We can inspect compositor capabilities here later.
     *
     * For example:
     *
     *   maximize
     *   fullscreen
     *   minimize
     */
}

}