#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform ColorInfoWrapper
{
  float val;
};

void main()
{
  outColor = vec4(vec3(val), 1.0);
}
