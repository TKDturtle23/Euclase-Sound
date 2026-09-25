#include "VulkanCommandBuffer.h"

#include "vulkanPipeline.h"

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
    void VulkanCommandBuffer::PushConstant(
        ShaderStage stage,
        uint32_t offset,
        const void* data,
        size_t size, GraphicsPipeline* pipeline)
    {
        if (!pipeline)
            throw std::runtime_error("No pipeline bound");
        VulkanPipeline* vulkanPipeline = static_cast<VulkanPipeline*>(pipeline);
        vk::ShaderStageFlags shaderStage{};

        switch (stage)
        {
            case ShaderStage::Vertex:
                shaderStage = vk::ShaderStageFlagBits::eVertex;
                break;

            case ShaderStage::Fragment:
                shaderStage = vk::ShaderStageFlagBits::eFragment;
                break;

            case ShaderStage::Compute:
                shaderStage = vk::ShaderStageFlagBits::eCompute;
                break;

            case ShaderStage::AllGraphics:
                shaderStage = vk::ShaderStageFlagBits::eAllGraphics;
                break;
        }

        commandBuffer.pushConstants(
            vulkanPipeline->GetLayoutHandle(),
            shaderStage,
            offset,
            static_cast<uint32_t>(size),
            data
        );
    }

    void VulkanCommandBuffer::PushResource(const ShaderResource &resource, GraphicsPipeline* pipeline) {
        VulkanPipeline* vulkanPipeline = static_cast<VulkanPipeline*>(pipeline);
        if (!pipeline)
            throw std::runtime_error("No pipeline bound");

        vk::DescriptorType descriptorType{};

        switch (resource.type)
        {
            case ResourceType::UniformBuffer:
                descriptorType = vk::DescriptorType::eUniformBuffer;
                break;

            case ResourceType::StorageBuffer:
                descriptorType = vk::DescriptorType::eStorageBuffer;
                break;

            case ResourceType::Texture:
                descriptorType = vk::DescriptorType::eSampledImage;
                break;

            case ResourceType::Sampler:
                descriptorType = vk::DescriptorType::eSampler;
                break;

            case ResourceType::CombinedImageSampler:
                descriptorType = vk::DescriptorType::eCombinedImageSampler;
                break;
        }
        vk::WriteDescriptorSet write{};

        write.dstSet = VK_NULL_HANDLE;
        write.dstBinding = resource.binding;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = descriptorType;
        write.pImageInfo = nullptr;
        vk::DescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = ;
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;
        write.pBufferInfo = &bufferInfo;

        commandBuffer.pushDescriptorSetKHR(
            vk::PipelineBindPoint::eGraphics,
            vulkanPipeline->GetLayoutHandle(),
            0,
            write
        );
    }
}
