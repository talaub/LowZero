#version 450

struct ObjectInfo
{
  mat4 mvp;
  mat4 model_matrix;
  uint material_index;
  uint entity_index;
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
layout(location = 2) in vec3 in_SurfaceNormal;
layout(location = 3) in mat3 in_TBN;

layout(std140, set = 0, binding = 1) readonly buffer MaterialInfoWrapper
{
  MaterialInfo g_MaterialInfos[];
};

layout(set = 0, binding = 2) uniform sampler2D g_Texture2Ds[8];

layout(std140, set = 2, binding = 0) readonly buffer ObjectInfoWrapper
{
  ObjectInfo u_RenderObjects[];
};

layout(location = 0) out vec4 o_Albedo;
layout(location = 1) out vec4 o_SurfaceNormal;
layout(location = 2) out vec4 o_Normal;
layout(location = 3) out vec4 o_Metalness;
layout(location = 4) out vec4 o_Roughness;
layout(location = 5) out uvec4 o_EntityIndex;

void main()
{
  uint l_MaterialIndex = u_RenderObjects[in_InstanceId].material_index;

  uint l_AlbedoTextureId = uint(g_MaterialInfos[l_MaterialIndex].v1.x);
  uint l_NormalTextureId = uint(g_MaterialInfos[l_MaterialIndex].v1.y);
  uint l_RoughnessTextureId = uint(g_MaterialInfos[l_MaterialIndex].v1.z);
  uint l_MetalnessTextureId = uint(g_MaterialInfos[l_MaterialIndex].v1.w);

  o_Albedo = vec4(
      texture(g_Texture2Ds[l_AlbedoTextureId], in_TextureCoordinates).xyz, 1.0);

  o_SurfaceNormal = vec4(vec3((in_SurfaceNormal.x + 1.0) / 2.0,
                              (in_SurfaceNormal.y + 1.0) / 2.0,
                              (in_SurfaceNormal.z + 1.0) / 2.0),
                         1.0);

  vec3 l_Normal =
      texture(g_Texture2Ds[l_NormalTextureId], in_TextureCoordinates).xyz;
  l_Normal = 2 * l_Normal - 1;

  l_Normal = normalize(in_TBN * l_Normal);
  // l_Normal = in_SurfaceNormal;
  o_Normal =
      vec4(normalize(vec3((l_Normal.x + 1.0) / 2.0, (l_Normal.y + 1.0) / 2.0,
                          (l_Normal.z + 1.0) / 2.0)),
           1.0);

  o_Metalness = vec4(
      vec3(
          texture(g_Texture2Ds[l_MetalnessTextureId], in_TextureCoordinates).x),
      1.0);
  o_Roughness = vec4(
      vec3(
          texture(g_Texture2Ds[l_RoughnessTextureId], in_TextureCoordinates).x),
      1.0);

  o_EntityIndex =
      uvec4(uvec3(u_RenderObjects[in_InstanceId].entity_index), 1.0);
}
