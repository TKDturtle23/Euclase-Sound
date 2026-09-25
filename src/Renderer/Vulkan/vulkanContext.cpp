#include "vulkanContext.h"

#include <cstring>
#include <iostream>
#include <vector>

namespace Euclase {

bool VulkanContext::Init(
    const std::vector<const char*>& platformExtensions,
    bool enableValidation)
{
    //
    // Vulkan loader
    //

    auto version = context.enumerateInstanceVersion();

    std::cout
        << "Vulkan instance version: "
        << VK_VERSION_MAJOR(version)
        << '.'
        << VK_VERSION_MINOR(version)
        << '.'
        << VK_VERSION_PATCH(version)
        << '\n';

    if (version < VK_API_VERSION_1_3)
    {
        std::cerr
            << "Vulkan 1.3 is required.\n";

        return false;
    }

    //
    // Application information
    //

    vk::ApplicationInfo applicationInfo{
        "Euclase",
        VK_MAKE_VERSION(1, 0, 0),
        "Euclase",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_3
    };

    //
    // Instance extensions
    //

    std::vector<const char*> extensions;

    extensions.push_back(
        VK_KHR_SURFACE_EXTENSION_NAME
    );

    extensions.insert(
        extensions.end(),
        platformExtensions.begin(),
        platformExtensions.end()
    );

    //
    // Validation layers
    //

    std::vector<const char*> layers;

    if (enableValidation)
    {
        const auto availableLayers =
            context.enumerateInstanceLayerProperties();

        bool validationFound = false;

        for (const auto& layer : availableLayers)
        {
            if (std::strcmp(
                    layer.layerName,
                    "VK_LAYER_KHRONOS_validation") == 0)
            {
                validationFound = true;
                break;
            }
        }

        if (!validationFound)
        {
            std::cerr
                << "VK_LAYER_KHRONOS_validation "
                   "is not available.\n";

            return false;
        }

        layers.push_back(
            "VK_LAYER_KHRONOS_validation"
        );
    }

    //
    // Instance creation
    //

    vk::InstanceCreateInfo createInfo{
        {},
        &applicationInfo,
        layers,
        extensions
    };

    try
    {
        instance = vk::raii::Instance(
            context,
            createInfo
        );
    }
    catch (const vk::SystemError& error)
    {
        std::cerr
            << "Failed to create Vulkan instance: "
            << error.what()
            << '\n';

        return false;
    }

    std::cout
        << "Vulkan instance created successfully.\n";

    return true;
}

void VulkanContext::Destroy()
{
    instance = nullptr;
}

}