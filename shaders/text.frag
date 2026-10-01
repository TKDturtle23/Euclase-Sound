#version 450


layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 color;

layout(binding = 0) uniform sampler2D atlas;

void main() {
    float col = texture(atlas,uv).r;
    if (col == 0) {
        discard;
    }
    outColor = vec4(color.xyz, col);
}
