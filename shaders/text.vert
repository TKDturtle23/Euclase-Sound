#version 450

vec2 positions[6] = vec2[](
vec2(0.0, 0.0),
vec2(1.0, 0.0),
vec2(1.0, 1.0),

vec2(0.0, 0.0),
vec2(1.0, 1.0),
vec2(0.0, 1.0)
);

struct Glyph {
    vec2 location;
    vec2 size;
    vec4 Color;
    vec2 uv;
    vec2 uvSize;
};

layout(binding = 1, std430) readonly buffer Glyphs {
    Glyph glyphs[];
} box;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 color;

void main()
{
    Glyph glyph = box.glyphs[gl_InstanceIndex];

    vec2 position = positions[gl_VertexIndex];

    uv = glyph.uv + position * glyph.uvSize;

    position =
    vec2(-1.0, -1.0)
    + glyph.location
    + position * glyph.size;

    gl_Position = vec4(position, 0.0, 1.0);

    color = glyph.Color;
}