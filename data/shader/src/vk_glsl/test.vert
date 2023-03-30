#version 450

struct ObjectInfo
{
  mat4 mvp;
  uint material_index;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTextureCoordinates;
layout(location = 2) in vec3 inSurfaceNormal;

layout(location = 0) out uint o_InstanceId;
layout(location = 1) out vec2 o_TextureCoordinates;
layout(location = 2) out vec3 o_SurfaceNormal;

layout(std140, set = 1, binding = 0) readonly buffer ObjectInfoWrapper
{
  ObjectInfo u_RenderObjects[];
};

void main()
{
  o_InstanceId = gl_InstanceIndex;
  o_TextureCoordinates = inTextureCoordinates;
  o_SurfaceNormal = inSurfaceNormal;

  gl_Position = u_RenderObjects[gl_InstanceIndex].mvp * vec4(inPosition, 1.0);
}
