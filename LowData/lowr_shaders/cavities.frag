#version 450

layout(location = 0) out vec4 outColor;

#include "fullscreen_effect.glsl"

// Helper: unpack and normalize normal
vec3 getNormal(vec2 uv) {
    vec3 n = texture(VIEW_GBUFFER_NORMAL, uv).xyz * 2.0 - 1.0;
    return normalize(n);
}

void main() {
    float cavityStrength = 1.5;
    float cavityRadius = 2;
    float bias = 1.0;
    float curvaturePower = 1.5;

    vec2 fragUV = gl_FragCoord.xy * g_ViewInfo.dimensions.zw;

    vec2 texelSize = 1.0 / vec2(textureSize(VIEW_GBUFFER_NORMAL, 0));
    float radius = cavityRadius;

    vec3 centerNormal = getNormal(fragUV);
    float sumDiff = 0.0;
    int sampleCount = 0;

    // 8 directions sampling (can use Poisson disk or hexagonal for better quality)
    const vec2 offsets[8] = vec2[](
        vec2(1,0), vec2(-1,0),
        vec2(0,1), vec2(0,-1),
        vec2(1,1), vec2(-1,1),
        vec2(1,-1), vec2(-1,-1)
    );

    for (int i = 0; i < 8; i++) {
        vec2 sampleUV = fragUV + offsets[i] * texelSize * radius;
        vec3 sampleNormal = getNormal(sampleUV);
        float diff = 1.0 - dot(centerNormal, sampleNormal); // curvature-like metric
        sumDiff += diff;
        sampleCount++;
    }

    float curvature = sumDiff / float(sampleCount);

    // Adjust curvature to cavity effect
    float cavity = pow(curvature - bias, curvaturePower);

    // Blend: negative curvature -> darken, positive curvature -> lighten
    float intensity = clamp(cavity * cavityStrength, -1.0, 1.0);

    outColor = vec4(vec3(0.5 + intensity * 0.5), 1.0); // visualize as overlay

}