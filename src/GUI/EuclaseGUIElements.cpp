//
// Created by Loyal on 10/6/26.
//
#include "EuclaseGUI.h"
namespace Euclase {
        void EuclaseGUI::Text(
        std::string_view text,
        float size,
        vec4 color
    ) {
        const float contentStartX =
            currentWindow.position.x +
            currentWindow.window->borderSize.x;

        /*
         * If SameLine() was not called, finish the previous
         * line and start a new one.
         */
        if (!currentWindow.sameLine) {
            currentWindow.lineStartY +=
                currentWindow.lineHeight;

            currentWindow.lineHeight = 0.0f;

            currentWindow.lineEndX =
                contentStartX;
        }

        /*
         * If SameLine() was called, continue from the
         * previous item's right edge.
         */
        currentWindow.pointer = {
            currentWindow.lineEndX +
            (
                currentWindow.sameLine
                    ? currentWindow.sameLineSpacing
                    : 0.0f
            ),

            currentWindow.lineStartY
        };

        currentWindow.sameLine = false;


        const float itemStartX =
            currentWindow.pointer.x;



        GlyphConstant glyphConstant;




        for (auto c : text) {
            auto glyph = font->GetGlyph(c);

            glyphConstant.location = {
                PixelsToUnits(
                    currentWindow.pointer.x +
                    glyph.bearing.x * size,
                    true
                ),

                PixelsToUnits(
                    currentWindow.pointer.y -
                    glyph.bearing.y * size +
                    font->GetFontSize() * size,
                    false
                )
            };

            glyphConstant.size = {
                PixelsToUnits(
                    glyph.size.x * size
                ),

                PixelsToUnits(
                    glyph.size.y * size,
                    false
                )
            };

            glyphConstant.Color = color;
            glyphConstant.uv = glyph.uv;
            glyphConstant.uvSize = glyph.uvSize;
            currentWindow.renderData.texts.push_back(glyphConstant);



            currentWindow.pointer.x +=
                glyph.advance * size;
        }


        const float width =
            currentWindow.pointer.x -
            itemStartX;

         float height =
            font->GetFontSize() * size;
        height += 8;


        currentWindow.lineEndX =
            currentWindow.pointer.x;

        currentWindow.lineHeight =
            std::max(
                currentWindow.lineHeight,
                height
            );


        // Update content size immediately.
        currentWindow.contentSize.x =
            std::max(
                currentWindow.contentSize.x,
                currentWindow.lineEndX -
                contentStartX
            );

        currentWindow.contentSize.y =
            std::max(
                currentWindow.contentSize.y,
                currentWindow.lineStartY +
                currentWindow.lineHeight -
                (
                    currentWindow.position.y +
                    currentWindow.window->borderSize.y
                )
            );
    }


    void EuclaseGUI::drawText(
        std::string_view text,
        float size,
        vec4 color
    ) {
        textPipeline->Bind(Buffer);

        GlyphConstant glyphConstant;

        Buffer->PushResource(
            FontAtlas,
            textPipeline.get()
        );


        for (auto c : text) {
            auto glyph = font->GetGlyph(c);

            glyphConstant.location = {
                PixelsToUnits(
                    currentWindow.pointer.x +
                    glyph.bearing.x * size,
                    true
                ),

                PixelsToUnits(
                    currentWindow.pointer.y -
                    glyph.bearing.y * size +
                    font->GetFontSize() * size,
                    false
                )
            };

            glyphConstant.size = {
                PixelsToUnits(
                    glyph.size.x * size
                ),

                PixelsToUnits(
                    glyph.size.y * size,
                    false
                )
            };

            glyphConstant.Color = color;
            glyphConstant.uv = glyph.uv;
            glyphConstant.uvSize = glyph.uvSize;


            currentWindow.renderData.texts.push_back(glyphConstant);

            currentWindow.pointer.x +=
                glyph.advance * size;
        }
    }


    bool EuclaseGUI::Button(
        std::string_view text,
        vec2 size,
        vec4 color
    ) {
        const float contentStartX =
            currentWindow.position.x +
            currentWindow.window->borderSize.x;

        /*
         * Start a new line unless SameLine() was called.
         */
        if (!currentWindow.sameLine) {
            currentWindow.lineStartY +=
                currentWindow.lineHeight;

            currentWindow.lineHeight = 0.0f;

            currentWindow.lineEndX =
                contentStartX;
        }


        currentWindow.pointer = {
            currentWindow.lineEndX +
            (
                currentWindow.sameLine
                    ? currentWindow.sameLineSpacing
                    : 0.0f
            ),

            currentWindow.lineStartY
        };

        currentWindow.sameLine = false;


        const vec2 position =
            currentWindow.pointer;


        const bool hovered =
            mousePosition.x >= position.x &&
            mousePosition.x <= position.x + size.x &&
            mousePosition.y >= position.y &&
            mousePosition.y <= position.y + size.y;


        const bool pressed =
            hovered &&
            mouseButtons[
                static_cast<size_t>(MouseButton::Left)
            ];


        vec4 buttonColor = color;


        if (pressed) {
            buttonColor.x *= 0.7f;
            buttonColor.y *= 0.7f;
            buttonColor.z *= 0.7f;
        }
        else if (hovered) {
            buttonColor.x *= 1.2f;
            buttonColor.y *= 1.2f;
            buttonColor.z *= 1.2f;
        }


        Box box;

        box.location =
            PixelsToUnits(position);

        box.size =
            PixelsToUnits(size);

        box.color =
            buttonColor;

        currentWindow.renderData.boxes.push_back(box);


        // Calculate text dimensions.
        float textWidth = 0.0f;

        for (char c : text) {
            textWidth +=
                font->GetGlyph(c).advance;
        }

        const float textHeight =
            font->GetFontSize();


        // Draw centered text without affecting layout.
        const vec2 oldPointer =
            currentWindow.pointer;

        currentWindow.pointer = {
            position.x +
                (size.x - textWidth) * 0.5f,

            position.y +
                (size.y - textHeight) * 0.5f
        };


        drawText(
            text,
            1.0f,
            {
                1.0f,
                1.0f,
                1.0f,
                1.0f
            }
        );


        currentWindow.pointer =
            oldPointer;


        // Update layout.
        currentWindow.lineEndX =
            position.x + size.x;

        currentWindow.lineHeight =
            std::max(
                currentWindow.lineHeight,
                size.y
            );


        currentWindow.contentSize.x =
            std::max(
                currentWindow.contentSize.x,
                currentWindow.lineEndX -
                contentStartX
            );

        currentWindow.contentSize.y =
            std::max(
                currentWindow.contentSize.y,
                currentWindow.lineStartY +
                currentWindow.lineHeight -
                (
                    currentWindow.position.y +
                    currentWindow.window->borderSize.y
                )
            );


        return hovered &&
               IsMouseReleased(
                   MouseButton::Left
               );
    }

}
