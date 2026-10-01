#include <cassert>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "GUI/Text.h"

namespace {

class TestTexture final : public Euclase::GraphicsTexture {
 public:
  TestTexture(uint32_t width, uint32_t height) : width_(width), height_(height) {}
  uint32_t GetWidth() const override { return width_; }
  uint32_t GetHeight() const override { return height_; }

 private:
  uint32_t width_;
  uint32_t height_;
};

class TestDevice final : public Euclase::GraphicsDevice {
 public:
  std::unique_ptr<Euclase::GraphicsPipeline> CreatePipeline(
      const Euclase::GraphicsPipelineDesc&) override { return {}; }
  std::shared_ptr<Euclase::GraphicsBuffer> CreateBuffer(
      size_t, Euclase::BufferUsage, Euclase::BufferMemory) override { return {}; }
  std::shared_ptr<Euclase::GraphicsTexture> CreateTexture(
      uint32_t width, uint32_t height, Euclase::TextureFormat format,
      const void* pixels, size_t size) override {
    assert(format == Euclase::TextureFormat::R8);
    assert(size == static_cast<size_t>(width) * height);
    pixels_.assign(static_cast<const uint8_t*>(pixels),
                   static_cast<const uint8_t*>(pixels) + size);
    return std::make_shared<TestTexture>(width, height);
  }

  const std::vector<uint8_t>& Pixels() const { return pixels_; }

 private:
  std::vector<uint8_t> pixels_;
};

}  // namespace

int main() {
  auto device = std::make_shared<TestDevice>();
  Euclase::Text text(device, EUCLASE_TEST_FONT, 32);

  assert(text.GetAtlasTexture());
  assert(text.GetAtlasWidth() == text.GetAtlasHeight());
  assert(device->Pixels().size() ==
         static_cast<size_t>(text.GetAtlasWidth()) * text.GetAtlasHeight());
  assert(text.GetAscender() > 0.0f);
  assert(text.GetLineHeight() > 0.0f);

  const Euclase::Glyph& a = text.FindGlyph('A');
  assert(a.width > 0 && a.height > 0 && a.advanceX > 0.0f);
  assert(a.u >= 0.0f && a.v >= 0.0f);
  assert(a.u + a.uSize <= 1.0f && a.v + a.vSize <= 1.0f);
  assert(&text.FindGlyph(0x03a9) == &text.FindGlyph('?'));
  assert(std::isfinite(text.GetKerning('A', 'V')));
}
