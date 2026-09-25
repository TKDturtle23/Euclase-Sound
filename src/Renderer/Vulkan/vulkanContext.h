#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace Euclase {

    class VulkanContext
    {
    public:
        bool Init(const std::vector<const char*>& platformExtensions,
            bool enableValidation);
        void Destroy();

        const vk::raii::Instance& GetInstance() const
        {
            return instance;
        }

    private:
        vk::raii::Context context;
        vk::raii::Instance instance{nullptr};
    };

}