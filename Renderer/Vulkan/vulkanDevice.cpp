#include "vulkanDevice.h"

#include <iostream>
#include <set>
#include <string>
#include <shaderc/shaderc.hpp>
#include "vulkanPipeline.h"

namespace Euclase {

bool VulkanDevice::Init(
    const vk::raii::Instance& instance,
    const vk::raii::SurfaceKHR& surface)
{
    if (!PickPhysicalDevice(instance, surface))
        return false;

    if (!CreateLogicalDevice())
        return false;

    return true;
}
    bool VulkanDevice::CheckRequiredFeatures(
        const vk::raii::PhysicalDevice& candidate) const
{
    auto features = candidate.getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan13Features
    >();

    const auto& coreFeatures = features.get<
        vk::PhysicalDeviceFeatures2
    >();

    const auto& vulkan13Features = features.get<
        vk::PhysicalDeviceVulkan13Features
    >();

    if (!coreFeatures.features.samplerAnisotropy) {
        std::cerr
            << "Device does not support sampler anisotropy.\n";

        return false;
    }


    if (!vulkan13Features.dynamicRendering) {
        std::cerr
            << "Device does not support dynamic rendering.\n";

        return false;
    }

    if (!vulkan13Features.synchronization2) {
        std::cerr
            << "Device does not support synchronization2.\n";

        return false;
    }


    return true;
}
void VulkanDevice::Destroy()
{
    graphicsQueue = nullptr;
    presentQueue = nullptr;

    device = nullptr;
    physicalDevice = nullptr;
}

    std::vector<uint32_t> CompileShader(
        const std::string& source,
        shaderc_shader_kind kind,
        const std::string& filename
    ) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    options.SetTargetEnvironment(
        shaderc_target_env_vulkan,
        shaderc_env_version_vulkan_1_3
    );

    auto result = compiler.CompileGlslToSpv(
        source,
        kind,
        filename.c_str(),
        options
    );

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        throw std::runtime_error(
            filename + ": " + result.GetErrorMessage()
        );
    }

    return {result.cbegin(), result.cend()};
}
    vk::raii::ShaderModule CreateShaderModule(
    vk::raii::Device& device,
    const std::vector<uint32_t>& spirv
) {
    vk::ShaderModuleCreateInfo info{
        {},
        spirv.size() * sizeof(uint32_t),
        spirv.data()
    };

    return vk::raii::ShaderModule(device, info);
}
    vk::Format VulkanDevice::ToVkFormat(TextureFormat format) {
    switch (format) {
        case TextureFormat::RGBA8:
            return vk::Format::eR8G8B8A8Srgb;

        case TextureFormat::BGRA8:
            return vk::Format::eB8G8R8A8Srgb;

        case TextureFormat::RGBA16F:
            return vk::Format::eR16G16B16A16Sfloat;

        case TextureFormat::Depth24Stencil8:
            return vk::Format::eD24UnormS8Uint;

        case TextureFormat::Depth32F:
            return vk::Format::eD32Sfloat;

        case TextureFormat::Undefined:
        default:
            return vk::Format::eUndefined;
    }
}
    std::unique_ptr<GraphicsPipeline>
    VulkanDevice::CreatePipeline(const GraphicsPipelineDesc& desc)
{
    auto vert = CompileShader(
        desc.vertexShader,
        shaderc_glsl_vertex_shader,
        "vert.vert"
    );

    auto frag = CompileShader(
        desc.fragmentShader,
        shaderc_glsl_fragment_shader,
        "frag.frag"
    );

    auto vertexModule = CreateShaderModule(device, vert);
    auto fragmentModule = CreateShaderModule(device, frag);

    const vk::Format colorFormat = ToVkFormat(desc.colorFormat);
    const vk::Format depthFormat = ToVkFormat(desc.depthFormat);

    return std::make_unique<VulkanPipeline>(
        device,
        vertexModule,
        fragmentModule,
        colorFormat,
        depthFormat
    );
}
bool VulkanDevice::PickPhysicalDevice(
    const vk::raii::Instance& instance,
    const vk::raii::SurfaceKHR& surface)
{
    const auto devices = instance.enumeratePhysicalDevices();

    if (devices.empty()) {
        std::cerr << "No Vulkan physical devices found.\n";
        return false;
    }

    std::optional<DeviceCandidate> bestCandidate;

    std::cout << "Available Vulkan devices:\n";

    for (const auto& candidate : devices) {
        const auto properties = candidate.getProperties();

        std::cout
            << "  " << properties.deviceName
            << " | ";

        switch (properties.deviceType) {
            case vk::PhysicalDeviceType::eDiscreteGpu:
                std::cout << "Discrete GPU";
                break;

            case vk::PhysicalDeviceType::eIntegratedGpu:
                std::cout << "Integrated GPU";
                break;

            case vk::PhysicalDeviceType::eVirtualGpu:
                std::cout << "Virtual GPU";
                break;

            case vk::PhysicalDeviceType::eCpu:
                std::cout << "CPU";
                break;

            default:
                std::cout << "Other";
                break;
        }

        std::cout << '\n';

        auto evaluated = EvaluateDevice(candidate, surface);

        if (!evaluated) {
            std::cout << "    Not suitable\n";
            continue;
        }

        std::cout
            << "    Score: "
            << evaluated->score
            << '\n';

        if (!bestCandidate ||
            evaluated->score > bestCandidate->score)
        {
            bestCandidate = std::move(evaluated);
        }
    }

    if (!bestCandidate) {
        std::cerr
            << "No suitable Vulkan physical device found.\n";

        return false;
    }

    physicalDevice = std::move(bestCandidate->device);

    graphicsQueueFamily =
        *bestCandidate->queues.graphics;

    presentQueueFamily =
        *bestCandidate->queues.present;

    const auto properties =
        physicalDevice.getProperties();

    std::cout
        << "Selected Vulkan device: "
        << properties.deviceName
        << '\n';

    std::cout
        << "Graphics queue family: "
        << graphicsQueueFamily
        << '\n';

    std::cout
        << "Present queue family: "
        << presentQueueFamily
        << '\n';

    return true;
}
    std::optional<VulkanDevice::DeviceCandidate>
    VulkanDevice::EvaluateDevice(
        const vk::raii::PhysicalDevice& candidate,
        const vk::raii::SurfaceKHR& surface) const
{
    const auto properties = candidate.getProperties();

    if (VK_API_VERSION_MAJOR(properties.apiVersion) < 1 ||
        (VK_API_VERSION_MAJOR(properties.apiVersion) == 1 &&
         VK_API_VERSION_MINOR(properties.apiVersion) < 3))
    {
        return std::nullopt;
    }

    const auto queues =
        FindQueueFamilies(candidate, surface);

    if (!queues.Complete())
        return std::nullopt;

    if (!CheckDeviceExtensionSupport(candidate))
        return std::nullopt;

    if (!CheckRequiredFeatures(candidate))
        return std::nullopt;

    return DeviceCandidate{
        candidate,
        queues,
        ScoreDevice(candidate)
    };
}
VulkanDevice::QueueFamilies VulkanDevice::FindQueueFamilies(
    const vk::raii::PhysicalDevice& candidate,
    const vk::raii::SurfaceKHR& surface) const
{
    QueueFamilies families;

    const auto queueProperties = candidate.getQueueFamilyProperties();

    for (uint32_t i = 0; i < queueProperties.size(); ++i) {
        const auto& properties = queueProperties[i];

        if (properties.queueFlags & vk::QueueFlagBits::eGraphics) {
            families.graphics = i;
        }

        const auto presentSupport =
            candidate.getSurfaceSupportKHR(i, *surface);

        if (presentSupport) {
            families.present = i;
        }

        if (families.Complete())
            break;
    }

    return families;
}
    int VulkanDevice::ScoreDevice(
        const vk::raii::PhysicalDevice& candidate) const
{
    const auto properties = candidate.getProperties();

    int score = 0;

    switch (properties.deviceType) {
        case vk::PhysicalDeviceType::eDiscreteGpu:
            score += 1000;
            break;

        case vk::PhysicalDeviceType::eIntegratedGpu:
            score += 500;
            break;

        case vk::PhysicalDeviceType::eVirtualGpu:
            score += 250;
            break;

        case vk::PhysicalDeviceType::eCpu:
            score += 100;
            break;

        default:
            break;
    }

    // Larger maximum texture/image dimensions generally indicate
    // a more capable GPU.
    score += static_cast<int>(
        properties.limits.maxImageDimension2D / 1024
    );

    // Favor devices with more available device-local memory.
    const auto memoryProperties =
        candidate.getMemoryProperties();

    vk::DeviceSize deviceLocalMemory = 0;

    for (uint32_t i = 0;
         i < memoryProperties.memoryHeapCount;
         ++i)
    {
        const auto& heap =
            memoryProperties.memoryHeaps[i];

        if (heap.flags &
            vk::MemoryHeapFlagBits::eDeviceLocal)
        {
            deviceLocalMemory =
                std::max(deviceLocalMemory, heap.size);
        }
    }

    // Add roughly one point per GiB.
    score += static_cast<int>(
        deviceLocalMemory /
        (1024ull * 1024ull * 1024ull)
    );

    return score;
}
bool VulkanDevice::CheckDeviceExtensionSupport(
    const vk::raii::PhysicalDevice& candidate) const
{
    const auto availableExtensions =
        candidate.enumerateDeviceExtensionProperties();

    std::set<std::string> required(
        requiredExtensions.begin(),
        requiredExtensions.end()
    );

    for (const auto& extension : availableExtensions) {
        required.erase(extension.extensionName);
    }

    return required.empty();
}

    bool VulkanDevice::CreateLogicalDevice()
{
    std::set<uint32_t> uniqueQueueFamilies = {
        graphicsQueueFamily,
        presentQueueFamily
    };

    constexpr float queuePriority = 1.0f;

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

    for (const uint32_t queueFamily : uniqueQueueFamilies) {
        queueCreateInfos.emplace_back(
            vk::DeviceQueueCreateFlags{},
            queueFamily,
            1,
            &queuePriority
        );
    }

    vk::PhysicalDeviceFeatures2 features2{};

    features2.features.samplerAnisotropy = VK_TRUE;

    vk::PhysicalDeviceVulkan13Features vulkan13Features{};

    vulkan13Features.dynamicRendering = VK_TRUE;
    vulkan13Features.synchronization2 = VK_TRUE;

    features2.pNext = &vulkan13Features;

    vk::DeviceCreateInfo createInfo{
        {},
        queueCreateInfos,
        {},
        requiredExtensions
    };
    createInfo.pNext = &features2;

    try {
        device = vk::raii::Device(
            physicalDevice,
            createInfo
        );
    }
    catch (const vk::SystemError& error) {
        std::cerr
            << "Failed to create Vulkan logical device: "
            << error.what()
            << '\n';

        return false;
    }

    graphicsQueue =
        device.getQueue(graphicsQueueFamily, 0);

    presentQueue =
        device.getQueue(presentQueueFamily, 0);

    return true;
}

}
