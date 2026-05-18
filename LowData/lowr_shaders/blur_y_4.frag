#version 450

#include "globals.glsl"

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform BlurParams {
    vec2 texelSize; // 1.0 / texture resolution
    uint inputTextureIndex;
    uint radius;
    float sigma;
    float stepScale;
} params;

const int MAX_RADIUS = 32;

void main() {
    vec2 texCoords = gl_FragCoord.xy * params.texelSize;

    int radius = int(min(params.radius, uint(MAX_RADIUS)));
    float sigma = max(params.sigma, 0.001);
    float twoSigmaSq = 2.0 * sigma * sigma;

    vec4 result = texture(TEX2D(params.inputTextureIndex), texCoords);
    float totalWeight = 1.0;

    for (int i = 1; i <= radius; ++i) {
        float y = float(i);
        float weight = exp(-(y * y) / twoSigmaSq);
        vec2 offset = vec2(0.0, params.texelSize.y * y * params.stepScale);
        result += texture(TEX2D(params.inputTextureIndex), texCoords + offset) * weight;
        result += texture(TEX2D(params.inputTextureIndex), texCoords - offset) * weight;
        totalWeight += weight * 2.0;
    }

    fragColor = result / totalWeight;
}
