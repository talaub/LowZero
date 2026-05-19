#version 450

layout(location = 0) out vec4 outColor;

#include "fullscreen_effect.glsl"

vec3 tonemap_neutral(vec3 p_Color)
{
  vec3 clamped = clamp(p_Color, vec3(0.0), vec3(1.0));
  vec3 shoulder =
      vec3(1.0) - exp(-(max(p_Color, vec3(1.0)) - vec3(1.0)));

  return mix(clamped, vec3(1.0), shoulder);
}

void main()
{
  vec2 uv = gl_FragCoord.xy * g_ViewInfo.dimensions.zw;
  vec3 hdrColor = texture(TEX2D(g_ViewInfo.textureIndices.z), uv).rgb;

  const float exposure = 1.0;
  vec3 mapped = tonemap_neutral(hdrColor * exposure);

  outColor = vec4(clamp(mapped, vec3(0.0), vec3(1.0)), 1.0);
}
