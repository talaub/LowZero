#version 450

struct ObjectInfo
{
  mat4 mvp;
  mat4 model_matrix;
  uint material_index;
  uint entity_index;
};

layout(location = 0) flat in uint in_InstanceId;
layout(location = 1) in vec2 in_TextureCoordinates;
layout(location = 2) in vec3 in_SurfaceNormal;

layout(std140, set = 2, binding = 0) readonly buffer ObjectInfoWrapper
{
  ObjectInfo u_RenderObjects[];
};

layout(std140, set = 2, binding = 1) readonly buffer ColorInfoWrapper
{
  vec4 u_Colors[];
};

layout(location = 0) out vec4 o_Output;

void main()
{
  o_Output = u_Colors[in_InstanceId];
}
