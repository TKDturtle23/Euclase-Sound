#include "VulkanSwapchain.h"
#include "vulkanDevice.h"

#include <algorithm>
#include <array>

namespace Euclase {

bool VulkanSwapchain::Init(
    const VulkanDevice& device,
    const vk::raii::SurfaceKHR& surface,
    uint32_t width,
    uint32_t height,
    vk::SwapchainKHR oldSwapchain
) {

    graphicsQueueFamily = device.GetGraphicsQueueFamily();
    presentQueueFamily = device.GetPresentQueueFamily();

    SupportDetails support = QuerySupport(device.GetPhysicalDevice(), surface);
    if (support.formats.empty() || support.presentModes.empty()) {
        return false;
    }

    surfaceFormat = ChooseSurfaceFormat(support.formats);
    presentMode = ChoosePresentMode(support.presentModes);
    extent = ChooseExtent(support.capabilities, width, height);

    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 &&
        imageCount > support.capabilities.maxImageCount) {
        imageCount = support.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.surface = *surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    // eTransferDst so you can blit/copy into the swapchain image (e.g. a
    // compute-shader-rendered image, or ImGui) in addition to rendering to
    // it directly as a color attachment.
    createInfo.imageUsage =
        vk::ImageUsageFlagBits::eColorAttachment |
        vk::ImageUsageFlagBits::eTransferDst;

    std::array<uint32_t, 2> queueFamilyIndices = {
        graphicsQueueFamily, presentQueueFamily
    };

    if (graphicsQueueFamily != presentQueueFamily) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = vk::True;
    createInfo.oldSwapchain = oldSwapchain;

    try {
        swapchain = vk::raii::SwapchainKHR(device.GetDevice(), createInfo);
    } catch (const vk::SystemError&) {
        return false;
    }

    images = swapchain.getImages();

    if (!CreateImageViews(device.GetDevice())) {
        return false;
    }

    return true;
}

void VulkanSwapchain::Destroy() {
    imageViews.clear();
    images.clear();
    swapchain = nullptr;
}

bool VulkanSwapchain::Recreate(
    const VulkanDevice& device,
    const vk::raii::SurfaceKHR& surface,
    uint32_t width,
    uint32_t height
) {
    // Keep the old swapchain alive (moved into oldHandle) until the new one
    // is created, per spec recommendation, then let it fall out of scope.
    vk::raii::SwapchainKHR oldHandle = std::move(swapchain);
    imageViews.clear();
    images.clear();

    bool ok = Init(device, surface, width, height, *oldHandle);

    // oldHandle destructs here, after the new swapchain exists.
    return ok;
}

bool VulkanSwapchain::AcquireNextImage(
    const vk::raii::Device& device,
    vk::Semaphore signalSemaphore,
    uint32_t& outImageIndex,
    bool& outOfDate,
    uint64_t timeout
) {
    outOfDate = false;

    vk::AcquireNextImageInfoKHR acquireInfo{};
    // Using the plain vkAcquireNextImageKHR path via raii's convenience
    // wrapper; swap to acquireNextImage2KHR if you need multi-GPU deviceMask.
    auto [result, imageIndex] = swapchain.acquireNextImage(
        timeout, signalSemaphore, nullptr
    );

    if (result == vk::Result::eErrorOutOfDateKHR) {
        outOfDate = true;
        return false;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        return false;
    }

    outImageIndex = imageIndex;
    return true;
}

void VulkanSwapchain::TransitionToColorAttachment(
    vk::CommandBuffer cmd,
    uint32_t imageIndex
) const {
    vk::ImageMemoryBarrier2 barrier{};
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
    barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
    barrier.oldLayout = vk::ImageLayout::eUndefined;
    barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.image = images[imageIndex];
    barrier.subresourceRange = vk::ImageSubresourceRange{
        vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
    };

    vk::DependencyInfo depInfo{};
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &barrier;

    cmd.pipelineBarrier2(depInfo);
}

void VulkanSwapchain::TransitionToPresent(
    vk::CommandBuffer cmd,
    uint32_t imageIndex
) const {
    vk::ImageMemoryBarrier2 barrier{};
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
    barrier.dstAccessMask = vk::AccessFlagBits2::eNone;
    barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
    barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.image = images[imageIndex];
    barrier.subresourceRange = vk::ImageSubresourceRange{
        vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
    };

    vk::DependencyInfo depInfo{};
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &barrier;

    cmd.pipelineBarrier2(depInfo);
}

vk::RenderingAttachmentInfo VulkanSwapchain::MakeColorAttachmentInfo(
    uint32_t imageIndex,
    vk::ClearColorValue clearColor
) const {
    vk::RenderingAttachmentInfo attachment{};
    attachment.imageView = *imageViews[imageIndex];
    attachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    attachment.loadOp = vk::AttachmentLoadOp::eClear;
    attachment.storeOp = vk::AttachmentStoreOp::eStore;
    attachment.clearValue.color = clearColor;
    return attachment;
}

VulkanSwapchain::SupportDetails VulkanSwapchain::QuerySupport(
    const vk::raii::PhysicalDevice& physicalDevice,
    const vk::raii::SurfaceKHR& surface
) const {
    SupportDetails details{};
    details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    details.formats = physicalDevice.getSurfaceFormatsKHR(*surface);
    details.presentModes = physicalDevice.getSurfacePresentModesKHR(*surface);
    return details;
}

vk::SurfaceFormatKHR VulkanSwapchain::ChooseSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& available
) const {
    for (const auto& format : available) {
        if (format.format == vk::Format::eB8G8R8A8Srgb &&
            format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return format;
        }
    }
    return available[0];
}

vk::PresentModeKHR VulkanSwapchain::ChoosePresentMode(
    const std::vector<vk::PresentModeKHR>& available
) const {
    // Mailbox gives low-latency triple buffering where supported; fall back
    // to FIFO, which is the only mode guaranteed by the spec.
    for (const auto& mode : available) {
        if (mode == vk::PresentModeKHR::eMailbox) {
            return mode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D VulkanSwapchain::ChooseExtent(
    const vk::SurfaceCapabilitiesKHR& capabilities,
    uint32_t width,
    uint32_t height
) const {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }

    vk::Extent2D actualExtent{width, height};
    actualExtent.width = std::clamp(
        actualExtent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width
    );
    actualExtent.height = std::clamp(
        actualExtent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height
    );
    return actualExtent;
}

bool VulkanSwapchain::CreateImageViews(const vk::raii::Device& device) {
    imageViews.clear();
    imageViews.reserve(images.size());

    for (const auto& image : images) {
        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.image = image;
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = surfaceFormat.format;
        viewInfo.components = vk::ComponentMapping{};
        viewInfo.subresourceRange = vk::ImageSubresourceRange{
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
        };

        try {
            imageViews.emplace_back(device, viewInfo);
        } catch (const vk::SystemError&) {
            return false;
        }
    }

    return true;
}

int VulkanSwapchain::GetImages() {
    return images.size();
}
}
