#version 450

struct ObjectInfo
{
  mat4 mvp;
  mat4 model_matrix;
  uint material_index;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTextureCoordinates;
layout(location = 2) in vec3 inSurfaceNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(std140, set = 2, binding = 0) readonly buffer ObjectInfoWrapper
{
  ObjectInfo u_RenderObjects[];
};

void main()
{
  gl_Position = u_RenderObjects[gl_InstanceIndex].mvp * vec4(inPosition, 1.0);
}
