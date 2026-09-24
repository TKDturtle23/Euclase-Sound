#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <vector>

namespace Euclase {

class VulkanDevice;

class VulkanSwapchain {
public:
    VulkanSwapchain() = default;
    ~VulkanSwapchain() = default;

    // extent is the fallback used when the surface reports {0xFFFFFFFF, 0xFFFFFFFF}
    // (i.e. "match whatever you ask for") — pass your window's current framebuffer size.
    bool Init(
        const VulkanDevice& device,
        const vk::raii::SurfaceKHR& surface,
        uint32_t width,
        uint32_t height,
        vk::SwapchainKHR oldSwapchain = nullptr
    );

    void Destroy();

    // Recreates the swapchain in place (e.g. on window resize / VK_ERROR_OUT_OF_DATE_KHR).
    // Reuses the current swapchain as oldSwapchain internally.
    bool Recreate(
        const VulkanDevice& device,
        const vk::raii::SurfaceKHR& surface,
        uint32_t width,
        uint32_t height
    );

    // Wraps vkAcquireNextImageKHR. Returns false (and sets outOfDate) if the
    // swapchain needs to be recreated before rendering can proceed.
    bool AcquireNextImage(
        const vk::raii::Device& device,
        vk::Semaphore signalSemaphore,
        uint32_t& outImageIndex,
        bool& outOfDate,
        uint64_t timeout = UINT64_MAX
    );

    // Issues the pre-rendering UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL barrier.
    // Call after vkCmdBeginRendering... actually call BEFORE vkCmdBeginRendering.
    void TransitionToColorAttachment(
        vk::CommandBuffer cmd,
        uint32_t imageIndex
    ) const;

    // Issues the post-rendering COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC_KHR barrier.
    // Call after vkCmdEndRendering, before present.
    void TransitionToPresent(
        vk::CommandBuffer cmd,
        uint32_t imageIndex
    ) const;

    // Builds a VkRenderingAttachmentInfo for the given image index, ready to
    // plug into VkRenderingInfo::pColorAttachments. loadOp/storeOp/clear are
    // yours to override after the call if you need something other than the
    // clear-then-store default.
    vk::RenderingAttachmentInfo MakeColorAttachmentInfo(
        uint32_t imageIndex,
        vk::ClearColorValue clearColor = vk::ClearColorValue{
            std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}
        }
    ) const;

    const vk::raii::SwapchainKHR& GetSwapchain() const {
        return swapchain;
    }

    vk::Image GetImage(uint32_t index) const {
        return images[index];
    }

    const vk::raii::ImageView& GetImageView(uint32_t index) const {
        return imageViews[index];
    }

    uint32_t GetImageCount() const {
        return static_cast<uint32_t>(images.size());
    }

    vk::Format GetFormat() const {
        return surfaceFormat.format;
    }

    vk::Extent2D GetExtent() const {
        return extent;
    }

private:
    struct SupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    SupportDetails QuerySupport(
        const vk::raii::PhysicalDevice& physicalDevice,
        const vk::raii::SurfaceKHR& surface
    ) const;

    vk::SurfaceFormatKHR ChooseSurfaceFormat(
        const std::vector<vk::SurfaceFormatKHR>& available
    ) const;

    vk::PresentModeKHR ChoosePresentMode(
        const std::vector<vk::PresentModeKHR>& available
    ) const;

    vk::Extent2D ChooseExtent(
        const vk::SurfaceCapabilitiesKHR& capabilities,
        uint32_t width,
        uint32_t height
    ) const;

    bool CreateImageViews(const vk::raii::Device& device);

private:
    vk::raii::SwapchainKHR swapchain{nullptr};

    // Non-owning: images come from vkGetSwapchainImagesKHR, lifetime is the
    // swapchain's. imageViews are owned (raii) and destroyed with this object.
    std::vector<vk::Image> images;
    std::vector<vk::raii::ImageView> imageViews;

    vk::SurfaceFormatKHR surfaceFormat{};
    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
    vk::Extent2D extent{};

    // Tracked purely so Recreate() knows what queue families to hand back in
    // to vkCreateSwapchainKHR without needing the VulkanDevice passed again
    // at Destroy() time.
    uint32_t graphicsQueueFamily = 0;
    uint32_t presentQueueFamily = 0;
};

}