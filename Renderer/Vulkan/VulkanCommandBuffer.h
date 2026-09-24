#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "../CommandBuffer.h"
namespace Euclase {

    // Thin wrapper around a single primary command buffer. Doesn't own the pool
    // it was allocated from — the pool must outlive this object.
    class VulkanCommandBuffer final : public CommandBuffer {
    public:
        VulkanCommandBuffer() = default;
        ~VulkanCommandBuffer() override = default;

        VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
        VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;
        VulkanCommandBuffer(VulkanCommandBuffer&&) = delete;
        VulkanCommandBuffer& operator=(VulkanCommandBuffer&&) = delete;

        bool Init(
            const vk::raii::Device& device,
            vk::CommandPool pool,
            vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary
        );

        void Destroy() {
            commandBuffer = nullptr;
        }
        void Reset() override {
           Reset({});
        }

        void Begin() override {
            Begin({});
        }
        void Draw(unsigned int vertices) override;
        void Reset(vk::CommandBufferResetFlags flags = {}) const {
            commandBuffer.reset(flags);
        }

        void Begin(vk::CommandBufferUsageFlags flags = {}) const {
            vk::CommandBufferBeginInfo beginInfo{};
            beginInfo.flags = flags;
            commandBuffer.begin(beginInfo);
        }

        void End() override {
            commandBuffer.end();
        }

        [[nodiscard]] vk::CommandBuffer Get() const {
            return *commandBuffer;
        }

        operator vk::CommandBuffer() const {
            return *commandBuffer;
        }

    private:
        vk::raii::CommandBuffer commandBuffer{nullptr};
    };

}