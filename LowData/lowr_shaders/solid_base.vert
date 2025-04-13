#version 450

layout(location = 0) out uint o_InstanceId;
layout(location = 1) out vec2 o_TextureCoordinates;
layout(location = 2) out vec3 o_SurfaceNormal;
layout(location = 3) out mat3 o_TBN;

struct Vertex {

	vec3 position;
  vec2 textoreCoordinates;
  vec3 normal;
  vec3 tangent;
  vec3 bitangent;
}; 

struct ViewInfo {
  mat4 viewMatrix;
  mat4 projectionMatrix;
  mat4 viewProjectionMatrix;
};

struct RenderObject {
  mat4 worldMatrix;
};

layout(std140, set = 0, binding = 0) readonly buffer VertexBufferWrapper
{
  Vertex g_Vertices[];
};

layout(std140, set = 0, binding = 1) readonly buffer RenderObjectWrapper
{
  RenderObject g_RenderObjects[];
};

layout(std140, set = 1, binding = 0) uniform ViewInfoBuffer{
  ViewInfo g_ViewInfo;
};

layout(push_constant) uniform constants
{
    uint renderObjectSlot;
} c_RenderEntry;

void main() 
{
  Vertex v = g_Vertices[gl_VertexIndex];

	//output the position of each vertex
	gl_Position = g_ViewInfo.viewProjectionMatrix * vec4(v.position, 1.0);

  o_InstanceId = 1;
  o_TextureCoordinates = v.textoreCoordinates;
  o_SurfaceNormal = v.normal;

  vec3 T = normalize(vec3(g_RenderObjects[c_RenderEntry.renderObjectSlot].worldMatrix *
                          vec4(v.tangent, 0.0)));
  vec3 B = normalize(vec3(g_RenderObjects[c_RenderEntry.renderObjectSlot].worldMatrix *
                          vec4(v.bitangent, 0.0)));
  vec3 N = normalize(vec3(g_RenderObjects[c_RenderEntry.renderObjectSlot].worldMatrix *
                          vec4(v.normal, 0.0)));
  mat3 TBN = mat3(T, B, N);
  o_TBN = TBN;

  // gl_Position = u_RenderObjects[gl_InstanceIndex].mvp * vec4(inPosition, 1.0);
}
