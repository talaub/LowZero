
// TODO: Keep these array sizes in sync with TextureSlots capacities
//       in LowRenderer2/vulkan/src/LowRendererVkTextureSlots.cpp
layout(set = 1, binding = 0) uniform texture2D  g_Texture2Ds[512];
layout(set = 1, binding = 1) uniform utexture2D g_UTexture2Ds[64];
layout(set = 1, binding = 2) uniform itexture2D g_ITexture2Ds[32];
layout(set = 1, binding = 3) uniform sampler    g_Samplers[1];
layout(set = 1, binding = 4) uniform sampler2D  g_EditorImages[128];

#define TEX2D(idx) sampler2D(g_Texture2Ds[idx], g_Samplers[0])

struct Material {
  vec4 val0;
  vec4 val1;
  vec4 val2;
  vec4 val3;
};

layout(std140, set = 0, binding = 2) readonly buffer MaterialBuffer{
  Material g_Materials[];
};

#define VIEW_GBUFFER_ALBEDO       TEX2D(g_ViewInfo.gbufferIndices.x)
#define VIEW_GBUFFER_NORMAL       TEX2D(g_ViewInfo.gbufferIndices.y)
#define VIEW_GBUFFER_DEPTH        TEX2D(g_ViewInfo.gbufferIndices.z)
#define VIEW_GBUFFER_VIEWPOSITION TEX2D(g_ViewInfo.gbufferIndices.w)
