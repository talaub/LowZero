struct Vertex {
  vec3 position;
  vec2 textureCoordinates;
  vec3 normal;
  vec3 tangent;
  vec3 bitangent;
}; 

layout(std140, set = 0, binding = 0) readonly buffer VertexBufferWrapper
{
  Vertex g_Vertices[];
};

layout(std140, set = 0, binding = 4) readonly buffer SkinnedVertexBufferAWrapper
{
  Vertex g_SkinnedVerticesA[];
};

layout(std140, set = 0, binding = 5) readonly buffer SkinnedVertexBufferBWrapper
{
  Vertex g_SkinnedVerticesB[];
};

#define VERTEX g_Vertices[gl_VertexIndex]
