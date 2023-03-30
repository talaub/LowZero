#version 450

struct ObjectInfo
{
  mat4 mvp;
  uint material_index;
};

struct MaterialInfo
{
  vec4 v0;
  vec4 v1;
  vec4 v2;
  vec4 v3;
};

layout(location = 0) flat in uint in_InstanceId;
layout(location = 1) in vec2 in_TextureCoordinates;

layout(std140, set = 0, binding = 1) readonly buffer MaterialInfoWrapper
{
  MaterialInfo g_MaterialInfos[];
};

layout(set = 0, binding = 2) uniform sampler2D g_Texture2Ds[8];

layout(std140, set = 1, binding = 0) readonly buffer ObjectInfoWrapper
{
  ObjectInfo u_RenderObjects[];
};

layout(location = 0) out vec4 outColor;
void main()
{
  uint l_MaterialIndex = u_RenderObjects[in_InstanceId].material_index;
  uint l_TextureId = uint(g_MaterialInfos[l_MaterialIndex].v1.x);

  outColor = g_MaterialInfos[l_MaterialIndex].v0;
  outColor =
      vec4(texture(g_Texture2Ds[l_TextureId], in_TextureCoordinates).xyz, 1.0);
}
