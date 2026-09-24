//
// Created by lstuart9 on 9/24/2026.
//

#include "RendererFactory.h"

#include "Vulkan/vulkanRenderer.h"

std::unique_ptr<Euclase::GraphicsRenderer> Euclase::GraphicsRendererFactory::Create(GraphicsAPI api) {
    switch (api)
    {
        case GraphicsAPI::Vulkan:
            return std::make_unique<vulkanRenderer>();

    }

    return nullptr;
}
