#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "vec.h"
#include "Renderer/GraphicsDevice.h"
namespace Euclase {
    struct Glyph {
        uint32_t codepoint;

        // Position/size of the glyph bitmap inside the atlas
        int atlasX;
        int atlasY;
        int width;
        int height;

        // FreeType positioning information
        int bearingX;
        int bearingY;
        int advanceX;
    };

    class Font {
    public:
        void Init(
            std::shared_ptr<Euclase::GraphicsDevice> device,
            std::string path,
            int size
        );

        struct GlyphInfo {
            vec2 uv;
            vec2 uvSize;

            vec2 size;
            vec2 bearing;

            float advance;
        };

        const GlyphInfo& GetGlyph(char character) const {
            return glyphs[static_cast<unsigned char>(character)];
        }

        std::shared_ptr<Euclase::GraphicsTexture> GetTexture() const {
            return texture;
        }
        int GetFontSize() const { return fontSize; }

    private:
        FT_Library library = nullptr;
        FT_Face face = nullptr;

        int fontSize = 0;

        std::shared_ptr<Euclase::GraphicsTexture> texture;

        GlyphInfo glyphs[128]{};
    };
}
