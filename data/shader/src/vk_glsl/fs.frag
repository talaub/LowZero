#version 450

layout(location = 0) out vec4 outColor;
layout(set = 1, binding = 0) uniform sampler2D u_Texture;

void main()
{
  vec2 coords =
      vec2((1.0 / 2000.0) * gl_FragCoord.x, (1.0 / 960.0) * gl_FragCoord.y);
  vec3 ocolor = texture(u_Texture, coords).xyz;
  outColor = vec4(ocolor, 1.0);
}
