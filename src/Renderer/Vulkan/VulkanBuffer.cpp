#include "VulkanBuffer.h"
#include "../GraphicsBuffer.h"
#include <cstring>
#include <stdexcept>

namespace Euclase {

    VulkanBuffer::VulkanBuffer(
        const vk::raii::PhysicalDevice& physicalDevice,
        const vk::raii::Device& device,
        size_t size,
        BufferUsage usage,
        BufferMemory memoryType
    )
        : physicalDevice(physicalDevice),
          device(device),
          size(size)
    {
        vk::BufferCreateInfo bufferInfo{
            .size = size,
            .usage = GetVulkanUsage(usage),
            .sharingMode = vk::SharingMode::eExclusive
        };

        buffer = vk::raii::Buffer(device, bufferInfo);

        vk::MemoryRequirements requirements =
            buffer.getMemoryRequirements();

        vk::MemoryAllocateInfo allocateInfo{
            .allocationSize = requirements.size,
            .memoryTypeIndex = FindMemoryType(
                requirements.memoryTypeBits,
                GetVulkanMemoryProperties(memoryType)
            )
        };

        memory = vk::raii::DeviceMemory(device, allocateInfo);

        buffer.bindMemory(*memory, 0);
    }


    uint32_t VulkanBuffer::FindMemoryType(
        uint32_t typeFilter,
        vk::MemoryPropertyFlags properties
    ) const
    {
        vk::PhysicalDeviceMemoryProperties memoryProperties =
            physicalDevice.getMemoryProperties();

        for (uint32_t i = 0;
             i < memoryProperties.memoryTypeCount;
             ++i)
        {
            if ((typeFilter & (1u << i)) &&
                (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error(
            "Failed to find suitable Vulkan memory type"
        );
    }


    vk::BufferUsageFlags VulkanBuffer::GetVulkanUsage(
        BufferUsage usage
    ) const
    {
        switch (usage)
        {
            case BufferUsage::Vertex:
                return vk::BufferUsageFlagBits::eVertexBuffer;

            case BufferUsage::Index:
                return vk::BufferUsageFlagBits::eIndexBuffer;

            case BufferUsage::Uniform:
                return vk::BufferUsageFlagBits::eUniformBuffer;

            case BufferUsage::Storage:
                return vk::BufferUsageFlagBits::eStorageBuffer;

            case BufferUsage::TransferSource:
                return vk::BufferUsageFlagBits::eTransferSrc;

            case BufferUsage::TransferDestination:
                return vk::BufferUsageFlagBits::eTransferDst;
        }

        throw std::runtime_error("Invalid BufferUsage");
    }


    vk::MemoryPropertyFlags VulkanBuffer::GetVulkanMemoryProperties(
        BufferMemory memory
    ) const
    {
        switch (memory)
        {
            case BufferMemory::GPUOnly:
                return vk::MemoryPropertyFlagBits::eDeviceLocal;

            case BufferMemory::CPUToGPU:
                return
                    vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent;

            case BufferMemory::GPUToCPU:
                return
                    vk::MemoryPropertyFlagBits::eHostVisible |
                    vk::MemoryPropertyFlagBits::eHostCoherent |
                    vk::MemoryPropertyFlagBits::eHostCached;
        }

        throw std::runtime_error("Invalid BufferMemory");
    }


    void* VulkanBuffer::Map()
    {
        if (mapped)
            return nullptr;

        void* data = memory.mapMemory(
            0,
            size
        );

        mapped = true;

        return data;
    }


    void VulkanBuffer::Unmap()
    {
        if (!mapped)
            return;

        memory.unmapMemory();

        mapped = false;
    }


    void VulkanBuffer::Write(
        const void* data,
        size_t writeSize,
        size_t offset
    )
    {
        if (offset + writeSize > size)
        {
            throw std::out_of_range(
                "VulkanBuffer::Write exceeds buffer size"
            );
        }

        void* mappedData = Map();

        if (!mappedData)
        {
            throw std::runtime_error(
                "Failed to map Vulkan buffer"
            );
        }

        std::memcpy(
            static_cast<std::byte*>(mappedData) + offset,
            data,
            writeSize
        );

        Unmap();
    }

}