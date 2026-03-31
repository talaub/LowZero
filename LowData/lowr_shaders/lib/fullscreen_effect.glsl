#include "globals.glsl"
#include "viewinfo.glsl"

vec3 get_world_position(vec2 p_PixelPosition)
{
    vec2 uv = p_PixelPosition.xy * g_ViewInfo.dimensions.zw;
    vec2 ndc = uv * 2.0 - 1.0;
    ndc.y = -ndc.y; // Vulkan NDC Y-flip

    float depth = texture(g_Texture2Ds[g_ViewInfo.gbufferIndices.z], uv).r;
    float clipZ = depth * 2.0 - 1.0;
    vec4 clipPos = vec4(ndc, clipZ, 1.0);

    // View-space position
    vec4 viewPos = g_ViewInfo.inverseProjectionMatrix * clipPos;
    viewPos /= viewPos.w;

    // World-space position
    vec4 worldPos = g_ViewInfo.inverseViewMatrix * viewPos;
    vec3 worldPosition = worldPos.xyz / worldPos.w;
    return worldPosition;
}

vec3 get_view_position(vec2 p_PixelPosition)
{
    vec2 uv = p_PixelPosition.xy * g_ViewInfo.dimensions.zw;
    vec2 ndc = uv * 2.0 - 1.0;
    ndc.y = -ndc.y; // Vulkan NDC Y-flip

    float depth = texture(g_Texture2Ds[g_ViewInfo.gbufferIndices.z], uv).r;
    float clipZ = depth * 2.0 - 1.0;
    vec4 clipPos = vec4(ndc, clipZ, 1.0);

    // View-space position
    vec4 viewPos = g_ViewInfo.inverseProjectionMatrix * clipPos;
    viewPos /= viewPos.w;

    return viewPos.xyz;
}

vec3 get_view_position(vec2 fragCoord, float depth) {
    vec2 ndc = (fragCoord * g_ViewInfo.dimensions.zw) * 2.0 - 1.0;
    vec4 clipPos = vec4(ndc, depth, 1.0);
    vec4 viewPos = g_ViewInfo.inverseProjectionMatrix * clipPos;
    return viewPos.xyz / viewPos.w;
}