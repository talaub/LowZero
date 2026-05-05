#version 450

layout(location = 0) out vec4 outColor;

#include "fullscreen_effect.glsl"

struct PointLight {
   vec4 transform; 
   vec4 color;
   uint valid;
};

struct DirectionalLight {
  vec4 color;
  vec3 direction;
};

layout(std430, set = 2, binding = 2) readonly buffer PointLightIndices{
  uint g_PointLightIndices[];
};

layout(std430, set = 2, binding = 3) readonly buffer PointLightViewBuffer{
  PointLight g_ViewPointLights[];
};

layout(std140, set = 2, binding = 6) uniform DirectionalLightBuffer{
  DirectionalLight g_DirectionalLight;
};

#define MAX_POINTLIGHTS_PER_CLUSTER 32

uint compute_z_cluster(float viewZ) {
    float logDepth = log(viewZ / g_ViewInfo.nearFarPlane.x) / log(g_ViewInfo.nearFarPlane.y / g_ViewInfo.nearFarPlane.x);
    uint clusterZ = uint(clamp(floor(logDepth * g_ViewInfo.lightClusters.z), 0.0, g_ViewInfo.lightClusters.z - 1));
    return clusterZ;
}

vec3 compute_lighting(vec3 fragPos, vec3 normal) {
    // Compute cluster index
    uvec2 tile = uvec2(gl_FragCoord.xy * g_ViewInfo.dimensions.zw * g_ViewInfo.lightClusters.xy);
    uint clusterZ = compute_z_cluster(-fragPos.z);

    uint clusterIndex = tile.x + tile.y * g_ViewInfo.lightClusters.x + clusterZ * g_ViewInfo.lightClusters.x * g_ViewInfo.lightClusters.y;

    // Fetch cluster's light list
    //uvec2 record = clusterRecords[clusterIndex];
    //uint offset = record.x;
    //uint count  = record.y;
    uint offset = clusterIndex * MAX_POINTLIGHTS_PER_CLUSTER;

    vec3 color = vec3(0.0);

    // Loop over lights
    for (uint i = 0; i < MAX_POINTLIGHTS_PER_CLUSTER; ++i) {
        uint lightIndex = g_PointLightIndices[offset + i];
        if (lightIndex >= 128) continue; // 129 = unused sentinel

        PointLight light = g_ViewPointLights[lightIndex];
        if (light.valid == 0) continue;
        if (light.color.a <= 0)continue;

        vec3 lightPosView = light.transform.xyz;
        float radius  = light.transform.w;
        vec3 lightColor = light.color.rgb;
        float intensity = light.color.a;

        // Simple diffuse lighting
        vec3 L = normalize(lightPosView - fragPos);
        float dist = length(lightPosView - fragPos);
        float att = clamp(1.0 - dist / radius, 0.0, 1.0);

        float diff = max(dot(normal, L), 0.0);
        color += lightColor * intensity * diff * att;
    }

    // Directional light
    if (g_DirectionalLight.color.a > 0.0) {
        vec3 lightColor = g_DirectionalLight.color.rgb;
        float intensity = g_DirectionalLight.color.a;

        // If direction points *where the light shines*, negate it to get surface->light vector.
        vec3 L = normalize(-g_DirectionalLight.direction.xyz);

        float diff = max(dot(normal, L), 0.0);
        color += lightColor * intensity * diff;
    }

    return color;
}

void main() {
    // UVs and depth
    vec2 uv = gl_FragCoord.xy * g_ViewInfo.dimensions.zw;

    float depth = texture(VIEW_GBUFFER_DEPTH, uv).r;

    #if 0
    depth = 1.0 - depth;
    #endif

    // Reconstruct view position
    vec3 fragPosView = texture(VIEW_GBUFFER_VIEWPOSITION, uv).xyz;

    // Fetch the albedo (diffuse color) from the albedo texture
    vec3 albedo = texture(VIEW_GBUFFER_ALBEDO, uv).rgb;
    vec3 normalWorld = normalize(texture(VIEW_GBUFFER_NORMAL, uv).rgb * 2.0 - 1.0);
    vec3 normalView = normalize(mat3(g_ViewInfo.viewMatrix) * normalWorld);

    // Compute lighting
    vec3 lighting = compute_lighting(fragPosView, normalView);

    // Sample SSAO texture
    float ao = texture(TEX2D(g_ViewInfo.textureIndices.x), uv).r;

    vec3 diffuse = lighting * albedo;
    vec3 ambient = vec3(0.03) * albedo;

    vec3 color = (diffuse + ambient) * ao; // AO darkens diffuse + ambient

    outColor = vec4(color, 1.0);
}
