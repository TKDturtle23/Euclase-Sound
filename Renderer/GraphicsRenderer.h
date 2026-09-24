#pragma once

#include "GraphicsTypes.h"
#include "CommandBuffer.h"

#include <memory>
#include "GraphicsDevice.h"
namespace Euclase {
    class Platform;

    class GraphicsRenderer {
    public:
        virtual ~GraphicsRenderer() = default;

        virtual bool Init(
        std::shared_ptr<Platform> platform, bool enableValidation, int width, int height
        ) = 0;

        virtual void Destroy() = 0;

        virtual bool BeginFrame() = 0;
        virtual void EndFrame() = 0;


        virtual std::shared_ptr<CommandBuffer> GetCommandBuffer() = 0;

        virtual std::shared_ptr<GraphicsDevice> GetDevice() = 0;
        virtual Extent2D GetExtent() const = 0;
        virtual TextureFormat GetSwapchainFormat() const = 0;
    };

}