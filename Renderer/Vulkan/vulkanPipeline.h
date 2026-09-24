#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <filesystem>
#include <vector>
#include "../GraphicsPipeline.h"
namespace Euclase {

    class VulkanPipeline final : public GraphicsPipeline
    {
    public:
        VulkanPipeline() = default;

        VulkanPipeline(
            const vk::raii::Device& device,
            vk::ShaderModule vertexShader,
            vk::ShaderModule fragmentShader,
            vk::Format colorFormat,
            vk::Format depthFormat = vk::Format::eUndefined
        );

        void Create(
            const vk::raii::Device& device,
            vk::ShaderModule vertexShader,
            vk::ShaderModule fragmentShader,
            vk::Format colorFormat,
            vk::Format depthFormat = vk::Format::eUndefined
        );

        [[nodiscard]]
        const vk::raii::Pipeline& Get() const noexcept;

        [[nodiscard]]
        const vk::raii::PipelineLayout& GetLayout() const noexcept;

        [[nodiscard]]
        vk::PipelineLayout GetLayoutHandle() const noexcept;

        void Bind(std::shared_ptr<CommandBuffer> buffer) override;
        void Bind(vk::CommandBuffer commandBuffer) const;

    private:
        vk::raii::PipelineLayout layout{nullptr};
        vk::raii::Pipeline pipeline{nullptr};
    };

} // namespace Euclase