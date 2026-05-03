#version 450

#include "vertex_outputs.glsl"

#include "debug_geometry_base.glsl"

void main()
{
    //output the position of each vertex
    vec4 l_WorldPosition = RENDER_OBJECT.worldMatrix * vec4(VERTEX.position, 1.0);
    gl_Position = g_ViewInfo.viewProjectionMatrix * l_WorldPosition;

#ifdef PICKING
    o_PickId = RENDER_OBJECT.pickId;
#else
    o_TextureCoordinates = VERTEX.textureCoordinates;
#endif
}
