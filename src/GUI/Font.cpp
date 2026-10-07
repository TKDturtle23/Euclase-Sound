#include "Font.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
namespace Euclase {
    void Font::Init(
    std::shared_ptr<Euclase::GraphicsDevice> device,
    std::string path,
    int size
) {
    FT_Error error = FT_Init_FreeType(&library);

    if (error) {
        std::cerr << "Failed to initialize FreeType library\n";
        return;
    }

    error = FT_New_Face(
        library,
        path.c_str(),
        0,
        &face
    );

    if (error == FT_Err_Unknown_File_Format) {
        std::cerr << "Font file is not a TrueType or OpenType font\n";
        return;
    }

    if (error) {
        std::cerr << "Failed to load font\n";
        return;
    }

    std::cout << "Loaded font: " << path << '\n';
    std::cout << "num glyphs: " << face->num_glyphs << '\n';

    error = FT_Set_Pixel_Sizes(face, 0, size);

    if (error) {
        std::cerr << "Failed to set font size\n";
        return;
    }

    fontSize = size;

    struct TemporaryGlyph {
        std::vector<uint8_t> bitmap;

        int bearingX = 0;
        int bearingY = 0;
        int advanceX = 0;

        int width = 0;
        int height = 0;
        int pitch = 0;
    };

    TemporaryGlyph temporary[128]{};

    int totalArea = 0;
    int maxGlyphWidth = 0;

    /*
     * Render every printable ASCII character.
     */
    for (int c = 32; c <= 126; ++c) {
        FT_UInt glyphIndex = FT_Get_Char_Index(
            face,
            static_cast<FT_ULong>(c)
        );

        error = FT_Load_Glyph(
            face,
            glyphIndex,
            FT_LOAD_DEFAULT
        );

        if (error) {
            std::cerr << "Failed to load glyph " << c << '\n';
            continue;
        }

        error = FT_Render_Glyph(
            face->glyph,
            FT_RENDER_MODE_NORMAL
        );

        if (error) {
            std::cerr << "Failed to render glyph " << c << '\n';
            continue;
        }

        const FT_GlyphSlot glyph = face->glyph;

        TemporaryGlyph& temp = temporary[c];

        temp.width = static_cast<int>(glyph->bitmap.width);
        temp.height = static_cast<int>(glyph->bitmap.rows);
        temp.pitch = static_cast<int>(glyph->bitmap.pitch);

        temp.bearingX = glyph->bitmap_left;
        temp.bearingY = glyph->bitmap_top;

        temp.advanceX =
            static_cast<int>(glyph->advance.x >> 6);

        /*
         * Copy the bitmap out of FreeType.
         *
         * glyph->bitmap belongs to the FreeType glyph slot and
         * will be overwritten when the next glyph is rendered.
         */
        if (temp.width > 0 && temp.height > 0) {
            temp.bitmap.resize(
                static_cast<size_t>(temp.pitch) *
                static_cast<size_t>(temp.height)
            );

            std::copy(
                glyph->bitmap.buffer,
                glyph->bitmap.buffer +
                    temp.bitmap.size(),
                temp.bitmap.begin()
            );
        }

        totalArea += temp.width * temp.height;

        maxGlyphWidth =
            std::max(maxGlyphWidth, temp.width);
    }

    /*
     * Calculate the atlas width dynamically.
     */
    constexpr int padding = 1;

    int atlasWidth = std::max(
        maxGlyphWidth + padding * 2,
        static_cast<int>(
            std::ceil(std::sqrt(
                static_cast<double>(totalArea)
            ))
        )
    );

    /*
     * Pack the glyphs into rows.
     */
    int x = padding;
    int y = padding;
    int rowHeight = 0;

        for (int c = 32; c <= 126; ++c) {
            const TemporaryGlyph& glyph = temporary[c];
            GlyphInfo& info = glyphs[c];

            // Metrics are needed for every glyph, including whitespace.
            info.size = {
                static_cast<float>(glyph.width),
                static_cast<float>(glyph.height)
            };
            info.bearing = {
                static_cast<float>(glyph.bearingX),
                static_cast<float>(glyph.bearingY)
            };
            info.advance = static_cast<float>(glyph.advanceX);

            // Empty bitmap: nothing to pack, uv stays zeroed.
            if (glyph.width == 0 || glyph.height == 0) {
                info.uv = {0.0f, 0.0f};
                info.uvSize = {0.0f, 0.0f};
                continue;
            }

            if (x + glyph.width + padding > atlasWidth) {
                x = padding;
                y += rowHeight + padding;
                rowHeight = 0;
            }

            info.uv = { static_cast<float>(x), static_cast<float>(y) };
            info.uvSize = {
                static_cast<float>(glyph.width),
                static_cast<float>(glyph.height)
            };

            x += glyph.width + padding;
            rowHeight = std::max(rowHeight, glyph.height);
        }

    const int bitmapWidth = atlasWidth;
    const int bitmapHeight = y + rowHeight + padding;

    /*
     * Allocate the atlas.
     *
     * One byte per pixel:
     *
     *   0   = transparent
     *   255 = fully opaque
     */
    std::vector<uint8_t> bitmap(
        static_cast<size_t>(bitmapWidth) *
        static_cast<size_t>(bitmapHeight),
        0
    );

    /*
     * Copy glyphs into the atlas.
     */
    for (int c = 32; c <= 126; ++c) {
        const TemporaryGlyph& glyph = temporary[c];

        if (glyph.width == 0 || glyph.height == 0)
            continue;

        const GlyphInfo& info = glyphs[c];

        const int atlasX =
            static_cast<int>(info.uv.x);

        const int atlasY =
            static_cast<int>(info.uv.y);

        for (int row = 0; row < glyph.height; ++row) {
            for (int col = 0; col < glyph.width; ++col) {
                const uint8_t value =
                    glyph.bitmap[
                        row * glyph.pitch + col
                    ];

                const int dstX = atlasX + col;
                const int dstY = atlasY + row;

                bitmap[
                    static_cast<size_t>(dstY) *
                    static_cast<size_t>(bitmapWidth) +
                    static_cast<size_t>(dstX)
                ] = value;
            }
        }
    }

    /*
     * Convert pixel coordinates into normalized UV coordinates.
     */
    for (int c = 32; c <= 126; ++c) {
        GlyphInfo& info = glyphs[c];

        info.uv.x /= static_cast<float>(bitmapWidth);
        info.uv.y /= static_cast<float>(bitmapHeight);

        info.uvSize.x /= static_cast<float>(bitmapWidth);
        info.uvSize.y /= static_cast<float>(bitmapHeight);
    }

    /*
     * Create the GPU texture.
     */
    texture = device->CreateTexture(
        static_cast<uint32_t>(bitmapWidth),
        static_cast<uint32_t>(bitmapHeight),
        Euclase::TextureFormat::R8,
        bitmap.data(),
        bitmap.size()
    );

    if (!texture) {
        std::cerr << "Failed to create font texture\n";
        return;
    }

    std::cout
        << "Font size: " << fontSize << '\n'
        << "Bitmap size: "
        << bitmapWidth << "x"
        << bitmapHeight << '\n';
}

    void Font::Destroy() {
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        texture = nullptr;
    }
}
