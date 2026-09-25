#version 450

vec2 positions[6] = vec2[](
    vec2(-0.5, -0.5),
    vec2( 0.5, -0.5),
    vec2( 0.5,  0.5),

    vec2(-0.5, -0.5),
    vec2( 0.5,  0.5),
    vec2(-0.5,  0.5)
);

vec3 colors[6] = vec3[](
    vec3(1.0, 0.2, 0.2),
    vec3(0.2, 1.0, 0.2),
    vec3(0.2, 0.2, 1.0),

    vec3(1.0, 0.2, 0.2),
    vec3(0.2, 0.2, 1.0),
    vec3(1.0, 0.2, 0.2)
);

layout(binding = 0) uniform Box {
    vec2 location;
    vec2 size;
    vec4 color;
} box;

layout(location = 0) out vec3 fragColor;

void main()
{
    vec2 position = positions[gl_VertexIndex];

    // Scale the unit box, then move it to its location.
    position = position * box.size + box.location;

    gl_Position = vec4(position, 0.0, 1.0);

    fragColor = box.color.rgb;
}