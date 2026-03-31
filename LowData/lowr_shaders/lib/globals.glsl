
layout(set = 1, binding = 0) uniform sampler2D g_Texture2Ds[512];
layout(set = 1, binding = 1) uniform sampler2D g_EditorImages[128];

struct Material {
  vec4 val0;
  vec4 val1;
  vec4 val2;
  vec4 val3;
};

layout(std140, set = 0, binding = 2) readonly buffer MaterialBuffer{
  Material g_Materials[];
};

#define VIEW_GBUFFER_ALBEDO g_Texture2Ds[g_ViewInfo.gbufferIndices.x]
#define VIEW_GBUFFER_NORMAL g_Texture2Ds[g_ViewInfo.gbufferIndices.y]
#define VIEW_GBUFFER_DEPTH g_Texture2Ds[g_ViewInfo.gbufferIndices.z]
#define VIEW_GBUFFER_VIEWPOSITION g_Texture2Ds[g_ViewInfo.gbufferIndices.w]
