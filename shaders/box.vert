#version 450

vec2 positions[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),

    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
);



layout(push_constant) uniform Box {
    vec2 location;
    vec2 size;
    vec4 color;
} box;

layout(location = 0) out vec3 fragColor;

void main()
{
    vec2 position = positions[gl_VertexIndex];


    position = vec2(-1.0, -1.0) + box.location.xy + vec2(
        position.x * box.size.x,
        position.y * box.size.y
    );


    gl_Position = vec4(position, 0, 1.0);

    fragColor = box.color.rgb;
}
