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

#define VERTEX g_Vertices[gl_VertexIndex]
