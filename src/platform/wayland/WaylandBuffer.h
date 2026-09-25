#pragma once

#include <wayland-client.h>

#include <cstddef>

namespace Euclase {

    class WaylandBuffer {
    public:
        WaylandBuffer() = default;
        ~WaylandBuffer();

        WaylandBuffer(const WaylandBuffer&) = delete;
        WaylandBuffer& operator=(const WaylandBuffer&) = delete;

        bool Create(
            wl_shm* shm,
            int width,
            int height
        );

        void Destroy();

        void* Data() const;

        int Width() const;
        int Height() const;

        bool IsReleased() const;

        wl_buffer* GetBuffer() const;

        // Wayland protocol callback
        static void BufferRelease(
            void* data,
            wl_buffer* buffer
        );

    private:
        int shmFd = -1;

        void* shmData = nullptr;
        size_t shmSize = 0;

        int width = 0;
        int height = 0;

        wl_shm_pool* shmPool = nullptr;
        wl_buffer* buffer = nullptr;

        bool bufferReleased = true;
    };

}