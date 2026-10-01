#pragma once
#include <vulkan/vulkan_raii.hpp>
#include "../GraphicsTexture.h"
#include "../GraphicsTypes.h"

namespace Euclase {
    class VulkanTexture final : public GraphicsTexture {
    public:
        VulkanTexture(const vk::raii::PhysicalDevice &, const vk::raii::Device &, vk::Queue, uint32_t, uint32_t,
                      TextureFormat, const void *, size_t);

        uint32_t GetWidth() const override { return width; }
        uint32_t GetHeight() const override { return height; }
        vk::ImageView GetView() const { return *view; }
        vk::Sampler GetSampler() const { return *sampler; }

    private:
        uint32_t FindMemory(uint32_t, vk::MemoryPropertyFlags) const;

        const vk::raii::PhysicalDevice &physical;
        const vk::raii::Device &device;
        uint32_t width, height;
        vk::raii::Image image{nullptr};
        vk::raii::DeviceMemory memory{nullptr};
        vk::raii::ImageView view{nullptr};
        vk::raii::Sampler sampler{nullptr};
    };
}
