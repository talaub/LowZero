#version 450

layout(location = 0) in vec3 inPosition;

layout(std140, set = 1, binding = 0) readonly buffer ObjectInfoWrapper
{
  mat4 u_RenderObjects[];
};

void main()
{
  gl_Position = u_RenderObjects[gl_InstanceIndex] * vec4(inPosition, 1.0);
}
