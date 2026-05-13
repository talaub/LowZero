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

struct ShadowTile {
  vec2 atlas_offset;
  vec2 atlas_scale;
};

layout(std430, set = 2, binding = 7) readonly buffer DirectionalShadowBuffer {
  mat4  dir_light_space[4];
  ShadowTile dir_tiles[4];
  vec4  dir_cascade_splits;
} g_DirShadow;

struct PointLightShadowInfo {
  mat4       light_space[6];
  ShadowTile tiles[6];
  uint       casts_shadow;
  uint       _pad0;
  uint       _pad1;
  uint       _pad2;
};

layout(std430, set = 2, binding = 8) readonly buffer PointShadowBuffer {
  PointLightShadowInfo g_PointLightShadows[];
};

layout(set = 2, binding = 9) uniform sampler2DShadow g_ShadowAtlas;

#define SHADOW_ATLAS_SIZE 4096.0

float sample_shadow_pcf(vec2 p_UV, float p_Depth, float p_Bias) {
  if (p_Depth <= 0.0 || p_Depth >= 1.0 ||
      any(lessThan(p_UV, vec2(0.0))) ||
      any(greaterThan(p_UV, vec2(1.0)))) {
    return 1.0;
  }

  float l_Shadow = 0.0;
  vec2  l_Texel  = 1.0 / vec2(SHADOW_ATLAS_SIZE);
  float l_Depth  = p_Depth - p_Bias;
  for (int x = -1; x <= 1; ++x) {
    for (int y = -1; y <= 1; ++y) {
      l_Shadow += texture(g_ShadowAtlas,
                          vec3(p_UV + vec2(x, y) * l_Texel, l_Depth));
    }
  }
  return l_Shadow / 9.0;
}

float compute_directional_shadow(vec3 p_FragPosView) {
  float l_Depth = -p_FragPosView.z;
  int   l_Cascade = 3;
  if      (l_Depth < g_DirShadow.dir_cascade_splits.x) l_Cascade = 0;
  else if (l_Depth < g_DirShadow.dir_cascade_splits.y) l_Cascade = 1;
  else if (l_Depth < g_DirShadow.dir_cascade_splits.z) l_Cascade = 2;

  vec4 l_WorldPos = g_ViewInfo.inverseViewMatrix * vec4(p_FragPosView, 1.0);
  vec4 l_LightSpace = g_DirShadow.dir_light_space[l_Cascade] * l_WorldPos;
  vec3 l_Proj = l_LightSpace.xyz / l_LightSpace.w;

  // XY: NDC [-1,1] -> [0,1]; Z already [0,1] (depth_zero_to_one)
  vec2 l_UV = l_Proj.xy * 0.5 + 0.5;
  if (l_Proj.z <= 0.0 || l_Proj.z >= 1.0 ||
      any(lessThan(l_UV, vec2(0.0))) ||
      any(greaterThan(l_UV, vec2(1.0)))) {
    return 1.0;
  }

  // Clamp to tile bounds
  ShadowTile l_Tile = g_DirShadow.dir_tiles[l_Cascade];
  vec2 l_AtlasUV = l_Tile.atlas_offset + l_UV * l_Tile.atlas_scale;

  return sample_shadow_pcf(l_AtlasUV, l_Proj.z, 0.0002);
}

int point_shadow_face(vec3 p_LightToFrag) {
  vec3 l_Abs = abs(p_LightToFrag);
  if (l_Abs.x >= l_Abs.y && l_Abs.x >= l_Abs.z) {
    return p_LightToFrag.x >= 0.0 ? 0 : 1;
  }
  if (l_Abs.y >= l_Abs.x && l_Abs.y >= l_Abs.z) {
    return p_LightToFrag.y >= 0.0 ? 2 : 3;
  }
  return p_LightToFrag.z >= 0.0 ? 4 : 5;
}

float compute_point_shadow(vec3 p_FragPosView, uint p_LightIndex) {
  if (p_LightIndex >= 128 ||
      g_PointLightShadows[p_LightIndex].casts_shadow == 0) {
    return 1.0;
  }

  vec4 l_WorldPos =
      g_ViewInfo.inverseViewMatrix * vec4(p_FragPosView, 1.0);
  vec3 l_LightPosWorld =
      (g_ViewInfo.inverseViewMatrix *
       vec4(g_ViewPointLights[p_LightIndex].transform.xyz, 1.0)).xyz;

  int l_Face = point_shadow_face(l_WorldPos.xyz - l_LightPosWorld);

  vec4 l_LightSpace =
      g_PointLightShadows[p_LightIndex].light_space[l_Face] *
      l_WorldPos;
  vec3 l_Proj = l_LightSpace.xyz / l_LightSpace.w;
  vec2 l_UV = l_Proj.xy * 0.5 + 0.5;

  if (l_Proj.z <= 0.0 || l_Proj.z >= 1.0 ||
      any(lessThan(l_UV, vec2(0.0))) ||
      any(greaterThan(l_UV, vec2(1.0)))) {
    return 1.0;
  }

  ShadowTile l_Tile =
      g_PointLightShadows[p_LightIndex].tiles[l_Face];
  vec2 l_AtlasUV = l_Tile.atlas_offset + l_UV * l_Tile.atlas_scale;

  return sample_shadow_pcf(l_AtlasUV, l_Proj.z, 0.002);
}

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
        float shadow = compute_point_shadow(fragPos, lightIndex);
        color += lightColor * intensity * diff * att * shadow;
    }

    // Directional light
    if (g_DirectionalLight.color.a > 0.0) {
        vec3 lightColor = g_DirectionalLight.color.rgb;
        float intensity = g_DirectionalLight.color.a;

        // If direction points *where the light shines*, negate it to get surface->light vector.
        vec3 L = normalize(-g_DirectionalLight.direction.xyz);

        float diff = max(dot(normal, L), 0.0);
        float shadow = compute_directional_shadow(fragPos);
        color += lightColor * intensity * diff * shadow;
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

    float ao = texture(TEX2D(g_ViewInfo.textureIndices.x), uv).r;

    vec3 diffuse = lighting * albedo;
    vec3 ambient = vec3(0.03) * albedo;

    vec3 color = (diffuse + ambient) * ao;

    // Cavity: ridge brightens edges, valley darkens crevices
    vec2 cavity = texture(TEX2D(g_ViewInfo.textureIndices.y), uv).rg;
    color *= (1.0 + cavity.r * 0.5);
    color *= (1.0 - cavity.g * 0.5);

    outColor = vec4(color, 1.0);
}
