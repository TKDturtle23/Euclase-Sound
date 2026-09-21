#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <vector>

namespace Euclase {

    class PlatformVulkan
    {
    public:
        virtual ~PlatformVulkan() = default;

        virtual std::vector<const char*> GetInstanceExtensions() const = 0;

        virtual vk::raii::SurfaceKHR CreateSurface(
            const vk::raii::Instance& instance
        ) const = 0;
    };

}