#version 450

vec2 positions[6] = vec2[](
vec2(0.0, 0.0),
vec2(1.0, 0.0),
vec2(1.0, 1.0),

vec2(0.0, 0.0),
vec2(1.0, 1.0),
vec2(0.0, 1.0)
);



layout(push_constant) uniform Glyph {
    vec2 location;
    vec2 size;
    vec4 Color;
    vec2 uv;
    vec2 uvSize;
} box;
layout(location = 0) out vec2 uv;
layout(location = 1) out vec4 color;
void main()
{
    vec2 position = positions[gl_VertexIndex];
    uv = box.uv + position * box.uvSize;

    position = vec2(-1.0, -1.0) + box.location + vec2(
    position.x * box.size.x,
    position.y * box.size.y
    );


    gl_Position = vec4(position, 0.0, 1.0);
    color = box.Color;

}
