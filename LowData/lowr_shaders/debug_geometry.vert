#version 450

layout(location = 0) out vec2 o_TextureCoordinates;

#include "debug_geometry_base.glsl"

void main()
{
    //output the position of each vertex
    vec4 l_WorldPosition = RENDER_OBJECT.worldMatrix * vec4(VERTEX.position, 1.0);
    gl_Position = g_ViewInfo.viewProjectionMatrix * l_WorldPosition;

    o_TextureCoordinates = VERTEX.textureCoordinates;
}

