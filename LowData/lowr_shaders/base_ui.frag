#version 450

#include "ui_fragment.glsl"

void main() 
{
  o_Color = texture(g_Texture2Ds[UI_DRAW_COMMAND.textureIndex], in_TextureCoordinates);
  //o_Color = texture(g_Texture2Ds[2], in_TextureCoordinates);
}
