#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "../GraphicsBuffer.h"

namespace Euclase {

    class VulkanBuffer final : public GraphicsBuffer {
    public:
        VulkanBuffer(
            const vk::raii::PhysicalDevice& physicalDevice,
            const vk::raii::Device& device,
            size_t size,
            BufferUsage usage,
            BufferMemory memory
        );

        ~VulkanBuffer() override = default;

        size_t GetSize() const override {
            return size;
        }

        void* Map() override;
        void Unmap() override;

        void Write(
            const void* data,
            size_t size,
            size_t offset = 0
        ) override;

        const vk::raii::Buffer& GetBuffer() const {
            return buffer;
        }

    private:
        uint32_t FindMemoryType(
            uint32_t typeFilter,
            vk::MemoryPropertyFlags properties
        ) const;

        vk::BufferUsageFlags GetVulkanUsage(
            BufferUsage usage
        ) const;

        vk::MemoryPropertyFlags GetVulkanMemoryProperties(
            BufferMemory memory
        ) const;

        const vk::raii::PhysicalDevice& physicalDevice;
        const vk::raii::Device& device;

        vk::raii::Buffer buffer{nullptr};
        vk::raii::DeviceMemory memory{nullptr};

        size_t size = 0;
        bool mapped = false;
    };

}