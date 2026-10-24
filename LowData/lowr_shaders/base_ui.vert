#version 450

#include "ui_vertex.glsl"

#if 0
void main() 
{
  vec2 l_AdjustedPosition = vec2(UI_DRAW_COMMAND.position.x, g_ViewInfo.dimensions.y - UI_DRAW_COMMAND.position.y);

    vec2 scaled = VERTEX.position.xy * UI_DRAW_COMMAND.size;

   // Rotate the sized vertex
    float cos_r = cos(UI_DRAW_COMMAND.rotation2D);
    float sin_r = sin(UI_DRAW_COMMAND.rotation2D);
    vec2 rotated = vec2(
        scaled.x * cos_r - scaled.y * sin_r,
        scaled.x * sin_r + scaled.y * cos_r
    );

    // Offset in screen space
    vec2 screen_pos = rotated + l_AdjustedPosition;

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

    // Size the quad before rotation to preserve its shape.
    vec2 scaled = VERTEX.position.xy * UI_DRAW_COMMAND.size;
    vec2 rotated = vec2(
        scaled.x * cos_r - scaled.y * sin_r,
        scaled.x * sin_r + scaled.y * cos_r
    );

    vec2 position = UI_DRAW_COMMAND.position;
    position.y = g_ViewInfo.dimensions.y - position.y;

    // Translate into UI space
    vec2 ui_pos = rotated + position;

    // Transform by UI view-projection matrix
    gl_Position = g_ViewInfo.uiViewProjectionMatrix * vec4(ui_pos, 0.0, 1.0);

    o_InstanceId = gl_InstanceIndex;
    o_TextureCoordinates = VERTEX.textureCoordinates;
}

#endif
