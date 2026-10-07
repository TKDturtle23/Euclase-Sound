//
// Created by Loyal on 10/6/26.
//
#include <algorithm>
#include <iostream>

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


        for (auto c: text) {
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


        for (auto c: text) {
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
        } else if (hovered) {
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

        for (char c: text) {
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
    struct OctaveNote {
        unsigned int Note; // A to G
        int Octave;
    };

    static OctaveNote intToOctaveNote(int note) {
        // note 0 = A4. A4 is 57 semitones above C0 (4*12 + 9).
        int fromC0 = note + 57;

        // Floor division/modulo that behaves for negatives
        int octave = fromC0 / 12;
        int pitchClass = fromC0 % 12;
        if (pitchClass < 0) { pitchClass += 12; octave -= 1; }

        OctaveNote n{};
        n.Octave = octave;
        n.Note   = pitchClass;   // C=0, C#=1, ... A=9, B=11
        return n;
    }
    /*
     * A: 0
     * A# 1
     * B 2
     * C 3
     * C# 4
     * D 5
     * D# 6
     * E 7
     * F 8
     * F# 9
     * G 10
     * G# 11
     */
    bool IsHalf(OctaveNote note) {
        switch (note.Note) {
            case 1:
            case 4:
            case 6:
            case 9:
            case 11:
                return true;
            default:
                return false;
        }
    }
    bool EuclaseGUI::HoveringBox(vec2 pos, vec2 size) {
        bool horizontal = mousePosition.x >= pos.x && mousePosition.x <= pos.x + size.x;
        bool vertical = mousePosition.y >= pos.y && mousePosition.y <= pos.y + size.y;
        return horizontal && vertical;
    }

    void EuclaseGUI::InstrumentNotes(
         int Lowest,  int Highest, Euclase::InstrumentNotes &Notes,
        const std::function<void(Euclase::InstrumentNotes &)> &callback,
        int NoteSize, float TimeWidth, float Height, int &scroll, float &scrollY, Quantization SelectQuant, bool quantize) {
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

        const int Width = (currentWindow.size.x - currentWindow.window->borderSize.x - currentWindow.window->padding.x) - currentWindow.pointer.x;
        constexpr int NotesWidth = 100;
        float m_TimeWidth = std::max(Width, 1);

        unsigned int visibleNotes = std::floor(Height / static_cast<float>(NoteSize)) - 1;
        int Middle = std::floor((Lowest + Highest) / 2);
        float ScrollSpeed = 0.25;

        // Vertical Scrolling
        if (visibleNotes <= Highest - Lowest)
            scroll += std::floor(-mouseScrollDelta.y * ScrollSpeed);

        if (Middle + scroll > Highest) {
            scroll = Highest - Middle;
        } else if ((Middle + scroll) - visibleNotes < Lowest && visibleNotes <= Highest - Lowest) {
            scroll = Lowest + visibleNotes;
        }


        // when scroll is Zero, the middle note in the range is the top of the visible area
        for (int i = 0; i < visibleNotes; i++) {
            int note = (Middle + scroll) - i;
            if (note < Lowest) {
                break;
            }
            OctaveNote octaveNote = intToOctaveNote(note);
            bool Black = IsHalf(octaveNote);
            float x = currentWindow.pointer.x;
            float y = currentWindow.pointer.y + (i * NoteSize);
            bool Hovered = HoveringBox({x, y}, {static_cast<float>(NotesWidth + Width), (float)NoteSize});
            bool Over = HoveringBox({x, y}, {static_cast<float>(NotesWidth), (float)NoteSize});
            vec4 Color1;
            vec4 Color2;
            if (Hovered) {
                Color1 = vec4{0.1f, 0.1f, 0.1f, 1.0f};
                Color2 = vec4{0.7f, 0.7f, 0.7f, 1.0f};
            } else {
                Color1 = vec4{0.0f, 0.0f, 0.0f, 1.0f};
                Color2 = vec4{.8f, .8f, .8f, 1.0f};
            }
            if (Black) {

                currentWindow.renderData.boxes.push_back(MakeBox({x, y}, {NotesWidth / 2, (float)NoteSize}, Color1));
                currentWindow.renderData.boxes.push_back(MakeBox({x + (NotesWidth / 2), y}, {NotesWidth / 2, (float)NoteSize}, Color2));
            } else {
                currentWindow.renderData.boxes.push_back(MakeBox({x, y}, {NotesWidth, (float)NoteSize}, Color2));
            }

            if (octaveNote.Note == 3) {
                WindowRenderData out;
                DrawTextAt(out, "C" + std::to_string(octaveNote.Octave), {currentWindow.pointer.x, y}, .8, {0.4, 0.4, 0.4, 1.0});
                currentWindow.renderData.texts.insert(currentWindow.renderData.texts.end(), out.texts.begin(), out.texts.end());
            }

        }
        for (int i = 0; i < visibleNotes; i++) {
            int note = (Middle + scroll) - i;
            if (note < Lowest) {
                break;
            }
            vec4 col = (i % 2) ? vec4{0.5f, 0.5f, 0.5f, 1.0f} : vec4{0.3f, 0.3f, 0.3f, 1.0f};
            float x = currentWindow.pointer.x + NotesWidth;
            float y = currentWindow.pointer.y + (i * NoteSize);
            bool Hovered = HoveringBox({x, y}, {static_cast<float>(Width), (float)NoteSize});
            if (Hovered) {
                col = vec4{0.25f, 0.25f, 0.25f, 1.0f};
            }
            currentWindow.renderData.boxes.push_back(MakeBox({x, y}, {m_TimeWidth, (float)NoteSize}, col));
        }



        currentWindow.pointer.y += visibleNotes * NoteSize;

    }
}
