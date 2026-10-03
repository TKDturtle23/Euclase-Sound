#include "VulkanTexture.h"

#include <cstring>
#include <stdexcept>

#include "VulkanBuffer.h"

namespace Euclase {
    namespace {
        vk::Format ToFormat(TextureFormat format) {
            switch (format) {
                case TextureFormat::R8: return vk::Format::eR8Unorm;
                case TextureFormat::RGBA8: return vk::Format::eR8G8B8A8Unorm;
                default: throw std::runtime_error("Unsupported Vulkan texture format");
            }
        }
    }

    VulkanTexture::VulkanTexture(const vk::raii::PhysicalDevice &physicalDevice,
                                 const vk::raii::Device &logicalDevice,
                                 vk::Queue queue, uint32_t textureWidth,
                                 uint32_t textureHeight, TextureFormat format,
                                 const void *pixels, size_t pixelSize)
        : physical(physicalDevice), device(logicalDevice), width(textureWidth), height(textureHeight) {
        VulkanBuffer staging(physical, device, pixelSize, BufferUsage::TransferSource,
                             BufferMemory::CPUToGPU);
        staging.Write(pixels, pixelSize);

        vk::ImageCreateInfo imageInfo{};
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.format = ToFormat(format);
        imageInfo.extent = vk::Extent3D{width, height, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = vk::SampleCountFlagBits::e1;
        imageInfo.tiling = vk::ImageTiling::eOptimal;
        imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;
        image = vk::raii::Image(device, imageInfo);
        const auto requirements = image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocation{
            requirements.size,
            FindMemory(requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
        };
        memory = vk::raii::DeviceMemory(device, allocation);
        image.bindMemory(*memory, 0);

        vk::CommandPoolCreateInfo poolInfo{};
        poolInfo.queueFamilyIndex = 0;
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
        vk::raii::CommandPool pool(device, poolInfo);
        vk::CommandBufferAllocateInfo commandInfo{*pool, vk::CommandBufferLevel::ePrimary, 1};
        vk::raii::CommandBuffers commands(device, commandInfo);
        vk::CommandBuffer command = *commands[0];
        command.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        vk::ImageMemoryBarrier barrier{};
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.image = *image;
        barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
        command.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);
        vk::BufferImageCopy copy{};
        copy.imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1};
        copy.imageExtent = vk::Extent3D{width, height, 1};
        command.copyBufferToImage(*staging.GetBuffer(), *image,
                                  vk::ImageLayout::eTransferDstOptimal, copy);
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);
        command.end();
        vk::SubmitInfo submit{};
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &command;
        vk::FenceCreateInfo fenceInfo{};
        vk::raii::Fence fence(device, fenceInfo);
        queue.submit(submit, *fence);
        auto wait = device.waitForFences(*fence, vk::True, UINT64_MAX);

        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.image = *image;
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = ToFormat(format);
        viewInfo.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
        view = vk::raii::ImageView(device, viewInfo);
        vk::SamplerCreateInfo samplerInfo{};
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;
        samplerInfo.addressModeU = samplerInfo.addressModeV =
                                   samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.maxLod = 0.0f;
        sampler = vk::raii::Sampler(device, samplerInfo);
    }

    uint32_t VulkanTexture::FindMemory(uint32_t typeBits, vk::MemoryPropertyFlags flags) const {
        const auto properties = physical.getMemoryProperties();
        for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
            if ((typeBits & (1u << i)) && (properties.memoryTypes[i].propertyFlags & flags) == flags) return i;
        throw std::runtime_error("No compatible Vulkan texture memory type");
    }
}
