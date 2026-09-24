#include "VulkanCommandBuffer.h"

namespace Euclase {

    bool VulkanCommandBuffer::Init(
        const vk::raii::Device& device,
        vk::CommandPool pool,
        vk::CommandBufferLevel level
    ) {
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.commandPool = pool;
        allocInfo.level = level;
        allocInfo.commandBufferCount = 1;

        try {
            // allocateCommandBuffers returns a vector of raii command buffers
            // tied to the pool; pull the one we asked for out of it.
            vk::raii::CommandBuffers buffers(device, allocInfo);
            commandBuffer = std::move(buffers[0]);
        } catch (const vk::SystemError&) {
            return false;
        }

        return true;
    }

    void VulkanCommandBuffer::Draw(unsigned int vertices) {
        commandBuffer.draw(vertices, 1, 0, 0);
    }
}
