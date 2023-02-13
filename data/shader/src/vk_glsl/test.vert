#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec2 o_TextureCoordinates;

void main()
{
  gl_Position = vec4(inPosition, 1.0);
  o_TextureCoordinates = inTexCoords;
}
