#include "vulkanRenderer.h"

#include "platform/vulkanPlatform.h"

namespace Euclase {

bool vulkanRenderer::Init(
std::shared_ptr<Platform> platform,
    bool enableValidation, int width, int height)
{
    windowWidth = width;
    windowHeight = height;
     platformVulkan = Euclase::PlatformVulkan::create(platform);
    const auto extensions =
        platformVulkan->GetInstanceExtensions();

    if (!context.Init(
            extensions,
            enableValidation))
    {
        return false;
    }

    try
    {
        surface = std::make_unique<vk::raii::SurfaceKHR>(
            platformVulkan->CreateSurface(
                context.GetInstance()
            )
        );
    }
    catch (const vk::SystemError&)
    {
        context.Destroy();
        return false;
    }
    device = std::make_shared<VulkanDevice>();
    if (!device->Init(context.GetInstance(), *surface)) {
        context.Destroy();
        return false;
    }

    if (!swapchain.Init(*device, *surface, width, height, nullptr)) {
        context.Destroy();
        return false;
    }

    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolInfo.queueFamilyIndex = device->GetGraphicsQueueFamily();

    try {
        commandPool = vk::raii::CommandPool(device->GetDevice(), poolInfo);
    } catch (const vk::SystemError&) {
        context.Destroy();
        return false;
    }

    commandBuffers.resize(kFramesInFlight);
    for (auto& cmd : commandBuffers) {
        cmd = std::make_shared<VulkanCommandBuffer>();
        if (!cmd->Init(device->GetDevice(), *commandPool)) {
            context.Destroy();
            return false;
        }
    }

    if (!CreateSyncObjects()) {
        context.Destroy();
        return false;
    }

    return true;
}

std::shared_ptr<GraphicsDevice> vulkanRenderer::GetDevice() {
    return device;
}



bool vulkanRenderer::CreateSyncObjects() {
    imageAvailableSemaphores.reserve(kFramesInFlight);
    renderFinishedSemaphores.reserve(kFramesInFlight);
    inFlightFences.reserve(kFramesInFlight);

    for (uint32_t i = 0; i < kFramesInFlight; ++i) {
        try {
            imageAvailableSemaphores.emplace_back(
                device->GetDevice(), vk::SemaphoreCreateInfo{}
            );
            renderFinishedSemaphores.emplace_back(
                device->GetDevice(), vk::SemaphoreCreateInfo{}
            );

            vk::FenceCreateInfo fenceInfo{};
            fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
            inFlightFences.emplace_back(device->GetDevice(), fenceInfo);
        } catch (const vk::SystemError&) {
            return false;
        }
    }

    return true;
}

void vulkanRenderer::Destroy()
{
    device->GetDevice().waitIdle();

    inFlightFences.clear();
    renderFinishedSemaphores.clear();
    imageAvailableSemaphores.clear();
    commandBuffers.clear();
    commandPool = nullptr;

    swapchain.Destroy();

    device->Destroy();

    surface.reset();

    context.Destroy();
}

void vulkanRenderer::RecreateSwapchain() {
    device->GetDevice().waitIdle();
    swapchain.Recreate(*device, *surface, windowWidth, windowHeight);
}

bool vulkanRenderer::BeginFrame() {
    vk::Fence fence = *inFlightFences[currentFrame];
    auto waitResult = device->GetDevice().waitForFences(fence, vk::True, UINT64_MAX);
    (void)waitResult;

    bool outOfDate = false;
    if (!swapchain.AcquireNextImage(
            device->GetDevice(),
            *imageAvailableSemaphores[currentFrame],
            currentImageIndex,
            outOfDate))
    {
        if (outOfDate) {
            RecreateSwapchain();
        }
        return false;
    }

    device->GetDevice().resetFences(fence);

    auto& cmd = commandBuffers[currentFrame];
    cmd->Reset();
    cmd->Begin();

    swapchain.TransitionToColorAttachment(cmd->Get(), currentImageIndex);

    auto colorAttachment = swapchain.MakeColorAttachmentInfo(currentImageIndex);
    vk::RenderingInfo renderingInfo{};
    renderingInfo.renderArea = vk::Rect2D{{0, 0}, swapchain.GetExtent()};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    cmd->Get().beginRendering(renderingInfo);

    vk::Viewport viewport{
        0.0f,
        0.0f,
        static_cast<float>(swapchain.GetExtent().width),
        static_cast<float>(swapchain.GetExtent().height),
        0.0f,
        1.0f
    };

    cmd->Get().setViewport(0, viewport);
    vk::Rect2D scissor{
        vk::Offset2D{0, 0},
        swapchain.GetExtent()
    };

    cmd->Get().setScissor(0, scissor);
    return true;
}

void vulkanRenderer::EndFrame() {
    auto& cmd = commandBuffers[currentFrame];

    cmd->Get().endRendering();
    swapchain.TransitionToPresent(cmd->Get(), currentImageIndex);
    cmd->End();

    vk::Semaphore waitSem = *imageAvailableSemaphores[currentFrame];
    vk::Semaphore signalSem = *renderFinishedSemaphores[currentFrame];
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::CommandBuffer cmdHandle = cmd->Get();

    vk::SubmitInfo submitInfo{};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &waitSem;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdHandle;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &signalSem;

    device->GetGraphicsQueue().submit(submitInfo, *inFlightFences[currentFrame]);

    vk::PresentInfoKHR presentInfo{};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &signalSem;
    vk::SwapchainKHR swapchainHandle = *swapchain.GetSwapchain();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchainHandle;
    presentInfo.pImageIndices = &currentImageIndex;

    auto presentResult = device->GetPresentQueue().presentKHR(presentInfo);
    if (presentResult == vk::Result::eErrorOutOfDateKHR ||
        presentResult == vk::Result::eSuboptimalKHR) {
        RecreateSwapchain();
    }

    currentFrame = (currentFrame + 1) % kFramesInFlight;
}

}