#pragma once

#include "vulkanContext.h"
#include "vulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanCommandBuffer.h"

#include <memory>
#include <vector>
#include "../GraphicsRenderer.h"
#include "../../platform/Platform.h"
#include "platform/vulkanPlatform.h"

namespace Euclase {


    class vulkanRenderer final : public GraphicsRenderer {
    public:
        vulkanRenderer() = default;
        ~vulkanRenderer() = default;

        bool Init(std::shared_ptr<Platform> platform, bool enableValidation, int width, int height) override;
        void Destroy() override;

        // Returns false if the frame should be skipped (swapchain was out of
        // date and is being recreated) — caller should just return early.
        bool BeginFrame();
        void EndFrame();

        std::shared_ptr<CommandBuffer> GetCommandBuffer() override {
            return commandBuffers[currentFrame];
        }

        Extent2D GetExtent() const override {
            const vk::Extent2D extent = swapchain.GetExtent();

            return {
                .width = extent.width,
                .height = extent.height
            };
        }

        TextureFormat GetSwapchainFormat() const override {
            switch (swapchain.GetFormat()) {
                case vk::Format::eR8G8B8A8Srgb:
                    return TextureFormat::RGBA8;

                case vk::Format::eB8G8R8A8Srgb:
                    return TextureFormat::BGRA8;

                case vk::Format::eR16G16B16A16Sfloat:
                    return TextureFormat::RGBA16F;

                case vk::Format::eD24UnormS8Uint:
                    return TextureFormat::Depth24Stencil8;

                case vk::Format::eD32Sfloat:
                    return TextureFormat::Depth32F;

                case vk::Format::eUndefined:
                default:
                    return TextureFormat::Undefined;
            }
        }
        std::shared_ptr<GraphicsDevice> GetDevice();
    private:
        static constexpr uint32_t kFramesInFlight = 2;

        bool CreateSyncObjects();
        void RecreateSwapchain();

    private:
        std::shared_ptr<Platform> m_platform;
        VulkanContext context;
        std::shared_ptr<VulkanDevice> device;
        VulkanSwapchain swapchain;
        std::unique_ptr<vk::raii::SurfaceKHR> surface;

        vk::raii::CommandPool commandPool{nullptr};
        std::vector<std::shared_ptr<VulkanCommandBuffer>> commandBuffers;

        std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
        std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
        std::vector<vk::raii::Fence> inFlightFences;

        uint32_t currentFrame = 0;
        uint32_t currentImageIndex = 0;

        int windowWidth = 0;
        int windowHeight = 0;

        std::shared_ptr<Euclase::PlatformVulkan> platformVulkan;
    };

}
