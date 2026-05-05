#version 450

#include "globals.glsl"

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform BlurParams {
    vec2 texelSize; // 1.0 / texture resolution
    uint inputTextureIndex;
} params;

// 5-tap Gaussian weights (approximation)
const float weights[5] = float[](0.227027, 0.316216, 0.070270, 0.070270, 0.316216);

void main() {
    vec2 texCoords = gl_FragCoord.xy * params.texelSize;

    vec4 result = texture(TEX2D(params.inputTextureIndex), texCoords) * weights[0];

    // Horizontal blur
    for (int i = 1; i < 3; ++i) {
        result += texture(TEX2D(params.inputTextureIndex), texCoords + vec2(params.texelSize.x * i, 0.0)) * weights[i];
        result += texture(TEX2D(params.inputTextureIndex), texCoords - vec2(params.texelSize.x * i, 0.0)) * weights[i];
    }

    fragColor = result;
}

