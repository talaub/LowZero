#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform ContextFrameInfoWrapper
{
  vec2 g_ContextInverseDimensions;
};

layout(set = 1, binding = 0) uniform sampler2D u_FinalImage;

void main()
{
  vec2 coords = vec2(g_ContextInverseDimensions.x * gl_FragCoord.x,
                     g_ContextInverseDimensions.y * gl_FragCoord.y);
  vec3 ocolor = texture(u_FinalImage, coords).xyz;
  outColor = vec4(ocolor, 1.0);
}
