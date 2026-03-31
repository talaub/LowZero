#version 450

#include "ui_vertex.glsl"

#if 0
void main() 
{
  vec2 l_AdjustedPosition = vec2(UI_DRAW_COMMAND.position.x, g_ViewInfo.dimensions.y - UI_DRAW_COMMAND.position.y);

   // Rotate object-space vertex
    float cos_r = cos(UI_DRAW_COMMAND.rotation2D);
    float sin_r = sin(UI_DRAW_COMMAND.rotation2D);
    vec2 rotated = vec2(
        VERTEX.position.x * cos_r - VERTEX.position.y * sin_r,
        VERTEX.position.x * sin_r + VERTEX.position.y * cos_r
    );

    // Scale to size in pixels, then offset in screen space
    vec2 screen_pos = rotated * UI_DRAW_COMMAND.size + l_AdjustedPosition;

    // Convert screen space to NDC
    vec2 ndc_pos = (screen_pos * g_ViewInfo.dimensions.zw) * 2.0 - 1.0;

    // Flip Y if your origin is top-left
    ndc_pos.y = -ndc_pos.y;

    gl_Position = vec4(ndc_pos, 0.0, 1.0);

    o_InstanceId = gl_InstanceIndex;
    //o_TextureCoordinates = UI_DRAW_COMMAND.uvRect.xy + (VERTEX.textureCoordinates * UI_DRAW_COMMAND.uvRect.zw);
    o_TextureCoordinates = VERTEX.textureCoordinates;
}
#else
void main()
{
    float cos_r = cos(UI_DRAW_COMMAND.rotation2D);
    float sin_r = sin(UI_DRAW_COMMAND.rotation2D);

    // Rotate local vertex in object space
    vec2 rotated = vec2(
        VERTEX.position.x * cos_r - VERTEX.position.y * sin_r,
        VERTEX.position.x * sin_r + VERTEX.position.y * cos_r
    );

    vec2 position = UI_DRAW_COMMAND.position;
    position.y = g_ViewInfo.dimensions.y - position.y;

    // Scale to pixel/UI size and translate into UI space
    vec2 ui_pos = rotated * UI_DRAW_COMMAND.size + position;

    // Transform by UI view-projection matrix
    gl_Position = g_ViewInfo.uiViewProjectionMatrix * vec4(ui_pos, 0.0, 1.0);

    o_InstanceId = gl_InstanceIndex;
    o_TextureCoordinates = VERTEX.textureCoordinates;
}

#endif
