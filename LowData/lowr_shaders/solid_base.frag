#version 450

#include "fragment_solid.glsl"

void main()
{
    vec3 l_SurfaceNormal = in_SurfaceNormal;

    Material l_Material = g_Materials[RENDER_OBJECT.materialIndex];

    uint l_TextureId = uint(l_Material.val0.a);

#ifdef HIGHLIGHT
    o_Highlight = c_RenderEntry.highlight;
    return;
#endif

    o_Albedo = vec4(l_Material.val0.rgb, 1.0);
    if (l_TextureId< 512){
      o_Albedo = vec4(texture(g_Texture2Ds[l_TextureId], in_TextureCoordinates).xyz, 1.0);
    }



    o_ViewPosition = vec4(in_ViewPosition, 1.0f);
    o_ObjectId = uint(RENDER_OBJECT.objectId);

    o_Normals = vec4(vec3((in_SurfaceNormal.x + 1.0) / 2.0,
                (in_SurfaceNormal.y + 1.0) / 2.0,
                (in_SurfaceNormal.z + 1.0) / 2.0),
            1.0);
}
