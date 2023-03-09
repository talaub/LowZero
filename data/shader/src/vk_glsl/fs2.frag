#version 450

layout(location = 0) out vec4 outColor;

void main()
{
  vec2 coords =
      vec2((1.0 / 2000.0) * gl_FragCoord.x, (1.0 / 960.0) * gl_FragCoord.y);
  outColor = vec4(vec3(0.2, 1.0, 0.3), 1.0);
}
