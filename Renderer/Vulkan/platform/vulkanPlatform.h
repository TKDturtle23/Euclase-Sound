#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <vector>
#include <memory>
#include "../../../platform/Platform.h"

namespace Euclase {

    class PlatformVulkan
    {
    public:
        virtual ~PlatformVulkan() = default;
        static std::shared_ptr<PlatformVulkan> create(std::shared_ptr<Platform> platform);

        virtual std::vector<const char*> GetInstanceExtensions() const = 0;

        virtual vk::raii::SurfaceKHR CreateSurface(
            const vk::raii::Instance& instance
        ) const = 0;
    };

}
