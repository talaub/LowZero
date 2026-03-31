#version 450

#include "ui_fragment.glsl"

#define BORDER_THICKNESS_PX 2

void main()
{

  // Convert pixel thickness to UV-space thickness using screen-space derivatives.
  vec2 l_UvPerPixel = fwidth(in_TextureCoordinates);
  vec2 l_BorderUv = vec2(float(BORDER_THICKNESS_PX)) * l_UvPerPixel;

  bool l_IsBorder =
      in_TextureCoordinates.x <= l_BorderUv.x ||
      in_TextureCoordinates.y <= l_BorderUv.y ||
      in_TextureCoordinates.x >= 1.0 - l_BorderUv.x ||
      in_TextureCoordinates.y >= 1.0 - l_BorderUv.y;

  if (!l_IsBorder) {
    discard;
  }

  o_Color = UI_DRAW_COMMAND.color;
}
