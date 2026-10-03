#pragma once

#include <cstdint>
#include "GraphicsBuffer.h"
#include "GraphicsTexture.h"
namespace Euclase {

    enum class GraphicsAPI {
        Vulkan,
        OpenGL,
        DirectX12,
        Metal
    };

    enum class TextureFormat {
        Undefined,

        RGBA8,
        R8,
        BGRA8,

        RGBA16F,

        Depth24Stencil8,
        Depth32F
    };

    struct Extent2D {
        uint32_t width = 0;
        uint32_t height = 0;
    };

    struct Color {
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        float a = 1.0f;
    };

    enum class ShaderStage {
        Vertex,
        Fragment,
        Compute,
        AllGraphics
    };

    enum class ResourceType {
        UniformBuffer,
        StorageBuffer,
        Texture,
        Sampler,
        CombinedImageSampler
    };


    struct ShaderResource {
        uint32_t binding;
        ResourceType type;
        ShaderStage stage;
        const void* data;
        size_t size;
        std::shared_ptr<GraphicsBuffer> buffer;
        std::shared_ptr<GraphicsTexture> texture;

        ShaderResource() = default;

        ShaderResource(int binding, ResourceType resource, ShaderStage stage, void* data, int size, const std::shared_ptr<Euclase::GraphicsBuffer> & buffer,
                       const std::shared_ptr<Euclase::GraphicsTexture> & shared = nullptr);
    };

    struct ShaderConstant {
        ShaderStage stage;
        uint32_t offset;

        const void* data;
        size_t size;

        ShaderConstant() = default;

        ShaderConstant(ShaderStage vertex, int offset, void* data, size_t size);
    };

    inline ShaderResource::ShaderResource(int binding, ResourceType resource, ShaderStage stage, void *data, int size,
        const std::shared_ptr<Euclase::GraphicsBuffer> &buffer,
        const std::shared_ptr<Euclase::GraphicsTexture> &shared) {
        this->binding = binding;
        this->type = resource;
        this->stage = stage;
        this->data = data;
        this->size = size;
        this->buffer = buffer;
        this->texture = shared;
    }

    inline ShaderConstant::ShaderConstant(ShaderStage vertex, int offset, void *data, size_t size) {
        stage = vertex;
        this->offset = offset;
        this->data = data;
        this->size = size;
    }
}
