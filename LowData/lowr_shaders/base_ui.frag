#version 450

#include "ui_fragment.glsl"

void main() 
{
  o_Color = texture(TEX2D(UI_DRAW_COMMAND.textureIndex), in_TextureCoordinates);
  //o_Color = texture(TEX2D(2), in_TextureCoordinates);
}
