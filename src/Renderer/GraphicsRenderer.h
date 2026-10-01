#pragma once

#include <memory>

#include "CommandBuffer.h"
#include "GraphicsDevice.h"
#include "GraphicsTypes.h"
namespace Euclase {
class Platform;

class GraphicsRenderer {
 public:
  virtual ~GraphicsRenderer() = default;

  virtual bool Init(std::shared_ptr<Platform> platform, bool enableValidation,
                    int width, int height) = 0;

  virtual void Destroy() = 0;

  virtual bool BeginFrame() = 0;
  virtual void EndFrame() = 0;
  virtual void WaitIdle() = 0;

  virtual std::shared_ptr<CommandBuffer> GetCommandBuffer() = 0;

  virtual std::shared_ptr<GraphicsDevice> GetDevice() = 0;
  virtual Extent2D GetExtent() const = 0;
  virtual TextureFormat GetSwapchainFormat() const = 0;
};

}  // namespace Euclase
