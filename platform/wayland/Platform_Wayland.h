#pragma once
#include "../Platform.h"

namespace Euclase {

    class WaylandDisplay;
    class WaylandWindow;

    class Platform_Wayland : public Platform{
    public:
        Platform_Wayland() = default;
        ~Platform_Wayland() override;

        bool CreateWindow(
            int width,
            int height,
            const char* title
        ) override;

        void Dispatch() override;

        void Disconnect() override;

        [[nodiscard]] WaylandDisplay* GetDisplay() const;
        [[nodiscard]] WaylandWindow* GetWindow() const;



    private:
        WaylandDisplay* display = nullptr;
        WaylandWindow* window = nullptr;
    };

}