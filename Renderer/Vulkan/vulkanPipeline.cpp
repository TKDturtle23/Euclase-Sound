#include "vulkanPipeline.h"

#include <array>
#include <stdexcept>

#include "VulkanCommandBuffer.h"

namespace Euclase {

VulkanPipeline::VulkanPipeline(
    const vk::raii::Device& device,
    vk::ShaderModule vertexShader,
    vk::ShaderModule fragmentShader,
    vk::Format colorFormat,
    vk::Format depthFormat)
{
    Create(
        device,
        vertexShader,
        fragmentShader,
        colorFormat,
        depthFormat
    );
}

void VulkanPipeline::Create(
    const vk::raii::Device& device,
    vk::ShaderModule vertexShader,
    vk::ShaderModule fragmentShader,
    vk::Format colorFormat,
    vk::Format depthFormat)
{
    //
    // Pipeline layout
    //

    vk::PipelineLayoutCreateInfo layoutInfo{};

    layout = vk::raii::PipelineLayout(
        device,
        layoutInfo
    );

    //
    // Shader stages
    //

    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
        vk::PipelineShaderStageCreateInfo{
            {},
            vk::ShaderStageFlagBits::eVertex,
            vertexShader,
            "main"
        },

        vk::PipelineShaderStageCreateInfo{
            {},
            vk::ShaderStageFlagBits::eFragment,
            fragmentShader,
            "main"
        }
    };

    //
    // Vertex input
    //

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

    //
    // Input assembly
    //

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
        {},
        vk::PrimitiveTopology::eTriangleList,
        VK_FALSE
    };

    //
    // Viewport / scissor
    //
    // These are dynamic, so the actual framebuffer size does not
    // need to be known when creating the pipeline.
    //

    vk::PipelineViewportStateCreateInfo viewportInfo{
        {},
        1,
        nullptr,
        1,
        nullptr
    };

    //
    // Rasterization
    //

    vk::PipelineRasterizationStateCreateInfo rasterizationInfo{
        {},
        VK_FALSE,
        VK_FALSE,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eClockwise,
        VK_FALSE,
        0.0f,
        0.0f,
        0.0f,
        1.0f
    };

    //
    // Multisampling
    //

    vk::PipelineMultisampleStateCreateInfo multisampleInfo{
        {},
        vk::SampleCountFlagBits::e1,
        VK_FALSE,
        1.0f,
        nullptr,
        VK_FALSE,
        VK_FALSE
    };

    //
    // Depth / stencil
    //

    vk::PipelineDepthStencilStateCreateInfo depthStencilInfo{};

    if (depthFormat != vk::Format::eUndefined)
    {
        depthStencilInfo.depthTestEnable = VK_TRUE;
        depthStencilInfo.depthWriteEnable = VK_TRUE;
        depthStencilInfo.depthCompareOp = vk::CompareOp::eLess;
    }

    //
    // Color blending
    //

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        VK_FALSE,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendInfo{
        {},
        VK_FALSE,
        vk::LogicOp::eCopy,
        1,
        &colorBlendAttachment
    };

    //
    // Dynamic state
    //

    constexpr std::array dynamicStates{
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateInfo{
        {},
        static_cast<uint32_t>(dynamicStates.size()),
        dynamicStates.data()
    };

    //
    // Dynamic rendering
    //
    // This replaces a VkRenderPass/VkFramebuffer pair.
    //

    vk::PipelineRenderingCreateInfo renderingInfo{
        {},
        1,
        &colorFormat,
        depthFormat,
        vk::Format::eUndefined
    };

    //
    // Graphics pipeline
    //

    vk::GraphicsPipelineCreateInfo pipelineInfo{
        {},
        static_cast<uint32_t>(shaderStages.size()),
        shaderStages.data(),
        &vertexInputInfo,
        &inputAssemblyInfo,
        nullptr,
        &viewportInfo,
        &rasterizationInfo,
        &multisampleInfo,
        &depthStencilInfo,
        &colorBlendInfo,
        &dynamicStateInfo,
        *layout,
        nullptr,
        0,
        nullptr,
        -1
    };

    pipelineInfo.pNext = &renderingInfo;

    pipeline = vk::raii::Pipeline(
        device,
        nullptr,
        pipelineInfo
    );
}

const vk::raii::Pipeline& VulkanPipeline::Get() const noexcept
{
    return pipeline;
}

const vk::raii::PipelineLayout& VulkanPipeline::GetLayout() const noexcept
{
    return layout;
}

vk::PipelineLayout VulkanPipeline::GetLayoutHandle() const noexcept
{
    return *layout;
}

void VulkanPipeline::Bind(std::shared_ptr<CommandBuffer> buffer) {
    auto* cmdBuffer = dynamic_cast<VulkanCommandBuffer*>(buffer.get());
    Bind(cmdBuffer->Get());
}

void VulkanPipeline::Bind(vk::CommandBuffer commandBuffer) const
{
    commandBuffer.bindPipeline(
        vk::PipelineBindPoint::eGraphics,
        *pipeline
    );
}

} // namespace Euclase