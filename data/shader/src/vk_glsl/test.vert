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

layout(location = 0) out uint o_InstanceId;
layout(location = 1) out vec2 o_TextureCoordinates;
layout(location = 2) out vec3 o_SurfaceNormal;
layout(location = 3) out mat3 o_TBN;

layout(std140, set = 1, binding = 0) readonly buffer ObjectInfoWrapper
{
  ObjectInfo u_RenderObjects[];
};

void main()
{
  o_InstanceId = gl_InstanceIndex;
  o_TextureCoordinates = inTextureCoordinates;
  o_SurfaceNormal =
      mat3(transpose(inverse(u_RenderObjects[gl_InstanceIndex].model_matrix))) *
      inSurfaceNormal;

  vec3 T = normalize(vec3(u_RenderObjects[gl_InstanceIndex].model_matrix *
                          vec4(inTangent, 0.0)));
  vec3 B = normalize(vec3(u_RenderObjects[gl_InstanceIndex].model_matrix *
                          vec4(inBitangent, 0.0)));
  vec3 N = normalize(vec3(u_RenderObjects[gl_InstanceIndex].model_matrix *
                          vec4(inSurfaceNormal, 0.0)));
  mat3 TBN = mat3(T, B, N);
  o_TBN = TBN;

  gl_Position = u_RenderObjects[gl_InstanceIndex].mvp * vec4(inPosition, 1.0);
}
