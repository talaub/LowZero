#version 450
#include "fragment_solid.glsl"

void main()
{
#ifdef SHADOW
  // depth written automatically
#elif defined(HIGHLIGHT)
  o_Highlight = c_RenderEntry.highlight;
#else
  Material l_Material = g_Materials[RENDER_OBJECT.materialIndex];

  // Layout (from calculate_offsets):
  //   shallow_color VECTOR3 -> val0.rgb
  //   fresnel_power FLOAT   -> val0.a   (packed into V3 padding slot)
  //   deep_color    VECTOR3 -> val1.rgb
  vec3  shallowColor = l_Material.val0.rgb;
  float fresnelPower = l_Material.val0.a;
  vec3  deepColor    = l_Material.val1.rgb;

  // Fresnel: camera is at origin in view space, so view dir = -viewPos
  vec3 viewDir  = normalize(-in_ViewPosition);
  vec3 normalVS = normalize(mat3(g_ViewInfo.viewMatrix) * in_SurfaceNormal);
  float NdotV   = max(dot(viewDir, normalVS), 0.0);
  float fresnel = pow(1.0 - NdotV, fresnelPower);

  vec3 waterColor = mix(deepColor, shallowColor, fresnel);

  o_Albedo      = vec4(waterColor, 1.0);
  o_ViewPosition = vec4(in_ViewPosition, 1.0);
  o_ObjectId    = uint(RENDER_OBJECT.objectId);

  vec3 n = in_SurfaceNormal;
  o_Normals = vec4((n.x + 1.0) / 2.0, (n.y + 1.0) / 2.0,
                   (n.z + 1.0) / 2.0, 1.0);
#endif
}
