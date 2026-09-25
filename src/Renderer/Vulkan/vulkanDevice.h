#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <optional>
#include <vector>

#include "../GraphicsBuffer.h"
#include "../GraphicsDevice.h"
namespace Euclase {

class VulkanDevice final : public GraphicsDevice {
public:
    VulkanDevice() = default;
    ~VulkanDevice() = default;

    bool Init(
        const vk::raii::Instance& instance,
        const vk::raii::SurfaceKHR& surface
    );

    void Destroy();

    const vk::raii::PhysicalDevice& GetPhysicalDevice() const {
        return physicalDevice;
    }

    const vk::raii::Device& GetDevice() const {
        return device;
    }

    vk::Queue GetGraphicsQueue() const {
        return graphicsQueue;
    }

    vk::Queue GetPresentQueue() const {
        return presentQueue;
    }

    uint32_t GetGraphicsQueueFamily() const {
        return graphicsQueueFamily;
    }

    uint32_t GetPresentQueueFamily() const {
        return presentQueueFamily;
    }

    vk::Format ToVkFormat(TextureFormat format);

    std::unique_ptr<GraphicsPipeline> CreatePipeline(
            const GraphicsPipelineDesc& desc
        ) override;
    std::shared_ptr<GraphicsBuffer> CreateBuffer(size_t size, BufferUsage usage, BufferMemory memory) override;
private:
    struct QueueFamilies {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;

        bool Complete() const {
            return graphics.has_value() &&
                   present.has_value();
        }
    };

    struct DeviceCandidate {
        vk::raii::PhysicalDevice device{nullptr};
        QueueFamilies queues;
        int score = 0;
    };

    bool PickPhysicalDevice(
        const vk::raii::Instance& instance,
        const vk::raii::SurfaceKHR& surface
    );

    std::optional<DeviceCandidate> EvaluateDevice(
        const vk::raii::PhysicalDevice& candidate,
        const vk::raii::SurfaceKHR& surface
    ) const;

    int ScoreDevice(
        const vk::raii::PhysicalDevice& candidate
    ) const;

    QueueFamilies FindQueueFamilies(
        const vk::raii::PhysicalDevice& candidate,
        const vk::raii::SurfaceKHR& surface
    ) const;

    bool CheckDeviceExtensionSupport(
        const vk::raii::PhysicalDevice& candidate
    ) const;

    bool CheckRequiredFeatures(
        const vk::raii::PhysicalDevice& candidate
    ) const;

    bool CreateLogicalDevice();

private:
    vk::raii::PhysicalDevice physicalDevice{nullptr};
    vk::raii::Device device{nullptr};

    vk::Queue graphicsQueue{nullptr};
    vk::Queue presentQueue{nullptr};

    uint32_t graphicsQueueFamily = 0;
    uint32_t presentQueueFamily = 0;

    std::vector<const char*> requiredExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME
    };
};

}