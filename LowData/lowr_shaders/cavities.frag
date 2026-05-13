#version 450

layout(location = 0) out vec2 outCavity; // R=ridge, G=valley

#include "fullscreen_effect.glsl"

layout(push_constant) uniform constants
{
    float radius;
    float ridgeStrength;
    float valleyStrength;
} params;

vec3 getViewNormal(vec2 uv) {
    vec3 worldN = texture(VIEW_GBUFFER_NORMAL, uv).xyz * 2.0 - 1.0;
    return normalize(mat3(g_ViewInfo.viewMatrix) * worldN);
}

void main() {
    vec2 uv = gl_FragCoord.xy * g_ViewInfo.dimensions.zw;

    float depth = texture(VIEW_GBUFFER_DEPTH, uv).r;
    if (depth >= 1.0) {
        outCavity = vec2(0.0);
        return;
    }

    vec2 texelSize = g_ViewInfo.dimensions.zw;
    float r = params.radius;

    vec3 nRight = getViewNormal(uv + vec2(texelSize.x * r, 0.0));
    vec3 nLeft  = getViewNormal(uv - vec2(texelSize.x * r, 0.0));
    vec3 nUp    = getViewNormal(uv + vec2(0.0, texelSize.y * r));
    vec3 nDown  = getViewNormal(uv - vec2(0.0, texelSize.y * r));

    // Divergence of the view-space normal field.
    // Positive = normals spread outward = convex ridge.
    // Negative = normals converge = concave valley.
    // Y is flipped (Vulkan NDC: uv y+ = screen down = view-space -y).
    float curvH = dot(nRight - nLeft, vec3(1.0, 0.0, 0.0));
    float curvV = dot(nDown  - nUp,   vec3(0.0, 1.0, 0.0));
    float signedCurvature = (curvH + curvV) * 0.5;

    float ridge  = clamp( signedCurvature * params.ridgeStrength,  0.0, 1.0);
    float valley = clamp(-signedCurvature * params.valleyStrength, 0.0, 1.0);

    outCavity = vec2(ridge, valley);
}
