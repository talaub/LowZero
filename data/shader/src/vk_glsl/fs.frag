#version 450

layout(location = 0) out vec4 outColor;
layout(binding = 0, set = 0, rgba32f) uniform image2D out_Color;
layout(set = 0, binding = 1) uniform sampler2D u_Texture;

void main()
{
  vec3 ocolor = texture(u_Texture, vec2(0.5, 0.5)).xyz;
  outColor = vec4(ocolor, 1.0);
}
