#version 450

#include "vertex.glsl"

void main()
{
  vec4 l_WorldPosition = RENDER_OBJECT.worldMatrix * vec4(VERTEX.position, 1.0);

#ifdef SHADOW
  gl_Position = c_RenderEntry.lightSpaceMatrix * l_WorldPosition;
#else
	gl_Position = g_ViewInfo.viewProjectionMatrix * l_WorldPosition;

#ifdef PICKING
  o_PickId = uint(RENDER_OBJECT.objectId);
#else
  o_InstanceId = 1;
  o_TextureCoordinates = VERTEX.textureCoordinates;
  mat3 normalMatrix = transpose(inverse(mat3(RENDER_OBJECT.worldMatrix)));
  o_SurfaceNormal = normalize(normalMatrix * VERTEX.normal);

  o_ViewPosition = (g_ViewInfo.viewMatrix*l_WorldPosition).xyz;

  vec3 T = normalize(vec3(RENDER_OBJECT.worldMatrix *
                          vec4(VERTEX.tangent, 0.0)));
  vec3 B = normalize(vec3(RENDER_OBJECT.worldMatrix *
                          vec4(VERTEX.bitangent, 0.0)));
  vec3 N = o_SurfaceNormal;
  mat3 TBN = mat3(T, B, N);
  o_TBN = TBN;
#endif
#endif
}
