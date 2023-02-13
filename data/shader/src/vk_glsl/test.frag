#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 in_TextureCoordinates;

layout(set = 0, binding = 0) uniform ColorInfoWrapper
{
  float val;
};

layout(set = 0, binding = 1) uniform sampler2D u_Img;

void main()
{
  // outColor = vec4(vec3(val), 1.0);
  outColor = texture(u_Img, in_TextureCoordinates);
}
