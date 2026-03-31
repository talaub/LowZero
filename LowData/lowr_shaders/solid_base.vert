#version 450

#include "vertex.glsl"

void main() 
{
	//output the position of each vertex
  vec4 l_WorldPosition = RENDER_OBJECT.worldMatrix * vec4(VERTEX.position, 1.0);
	gl_Position = g_ViewInfo.viewProjectionMatrix * l_WorldPosition;

  o_InstanceId = 1;
  o_TextureCoordinates = VERTEX.textureCoordinates;
  o_SurfaceNormal = VERTEX.normal;

  o_ViewPosition = (g_ViewInfo.viewMatrix*l_WorldPosition).xyz;

  vec3 T = normalize(vec3(RENDER_OBJECT.worldMatrix *
                          vec4(VERTEX.tangent, 0.0)));
  vec3 B = normalize(vec3(RENDER_OBJECT.worldMatrix *
                          vec4(VERTEX.bitangent, 0.0)));
  vec3 N = normalize(vec3(RENDER_OBJECT.worldMatrix *
                          vec4(VERTEX.normal, 0.0)));
  mat3 TBN = mat3(T, B, N);
  o_TBN = TBN;
}