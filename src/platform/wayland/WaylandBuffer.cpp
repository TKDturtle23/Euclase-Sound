#include "WaylandBuffer.h"

#include <iostream>

#include <cstring>
#include <cerrno>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <linux/memfd.h>
#include <sys/syscall.h>

namespace Euclase {

namespace {

const wl_buffer_listener bufferListener = {
    .release = WaylandBuffer::BufferRelease
};

    int CreateShmFile(size_t size)
    {
        const int fd = syscall(
            SYS_memfd_create,
            "euclase-wayland",
            MFD_CLOEXEC
        );

        if (fd == -1) {
            perror("memfd_create");
            return -1;
        }

        if (ftruncate(
            fd,
            static_cast<off_t>(size)
        ) == -1) {
            perror("ftruncate");
            close(fd);
            return -1;
        }

        return fd;
    }

}

WaylandBuffer::~WaylandBuffer()
{
    Destroy();
}

bool WaylandBuffer::Create(
    wl_shm* shm,
    int newWidth,
    int newHeight)
{
    if (!shm)
        return false;

    if (newWidth <= 0 || newHeight <= 0)
        return false;

    width = newWidth;
    height = newHeight;

    const int stride = width * 4;

    shmSize =
        static_cast<size_t>(stride) *
        static_cast<size_t>(height);

    shmFd = CreateShmFile(shmSize);

    if (shmFd == -1)
        return false;

    shmData = mmap(
        nullptr,
        shmSize,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        shmFd,
        0
    );

    if (shmData == MAP_FAILED) {
        shmData = nullptr;

        perror("mmap");

        close(shmFd);
        shmFd = -1;

        return false;
    }

    shmPool = wl_shm_create_pool(
        shm,
        shmFd,
        static_cast<int32_t>(shmSize)
    );

    if (!shmPool) {
        std::cerr
            << "Failed to create wl_shm_pool"
            << std::endl;

        Destroy();

        return false;
    }

    buffer = wl_shm_pool_create_buffer(
        shmPool,
        0,
        width,
        height,
        stride,
        WL_SHM_FORMAT_XRGB8888
    );

    if (!buffer) {
        std::cerr
            << "Failed to create wl_buffer"
            << std::endl;

        Destroy();

        return false;
    }

    wl_buffer_add_listener(
        buffer,
        &bufferListener,
        this
    );

    bufferReleased = true;

    return true;
}

void WaylandBuffer::Destroy()
{
    if (buffer) {
        wl_buffer_destroy(buffer);
        buffer = nullptr;
    }

    if (shmPool) {
        wl_shm_pool_destroy(shmPool);
        shmPool = nullptr;
    }

    if (shmData) {
        munmap(shmData, shmSize);

        shmData = nullptr;
    }

    if (shmFd != -1) {
        close(shmFd);

        shmFd = -1;
    }

    shmSize = 0;

    width = 0;
    height = 0;

    bufferReleased = true;
}

void* WaylandBuffer::Data() const
{
    return shmData;
}

int WaylandBuffer::Width() const
{
    return width;
}

int WaylandBuffer::Height() const
{
    return height;
}

bool WaylandBuffer::IsReleased() const
{
    return bufferReleased;
}

wl_buffer* WaylandBuffer::GetBuffer() const
{
    return buffer;
}

void WaylandBuffer::BufferRelease(
    void* data,
    wl_buffer* buffer)
{
    auto* waylandBuffer =
        static_cast<WaylandBuffer*>(data);

    waylandBuffer->bufferReleased = true;
}

}