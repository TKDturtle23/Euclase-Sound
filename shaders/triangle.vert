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
    vec2 location;  // Top-left position in pixels
    vec2 size;      // Width and height in pixels
    vec4 color;
    vec2 frameSize; // Frame width and height in pixels
} box;

layout(location = 0) out vec3 fragColor;

void main()
{
    // Calculate pixel position within the frame
    vec2 pixelPosition =
    positions[gl_VertexIndex] * box.size + box.location;

    // Convert pixel coordinates to normalized [0, 1] coordinates
    vec2 normalizedPosition = pixelPosition / box.frameSize;

    // Convert to Vulkan NDC: X [-1, 1], Y [-1, 1]
    vec2 position = vec2(
    normalizedPosition.x * 2.0 - 1.0,
    1.0 - normalizedPosition.y * 2.0
    );

    gl_Position = vec4(position, 0.0, 1.0);

    fragColor = box.color.rgb;
}