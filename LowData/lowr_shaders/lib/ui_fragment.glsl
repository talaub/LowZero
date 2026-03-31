layout(location = 0) flat in uint in_InstanceId;
layout(location = 1) in vec2 in_TextureCoordinates;

layout (location = 0) out vec4 o_Color;

#include "ui.glsl"

#define UI_DRAW_COMMAND g_UiDrawCommands[g_UiDrawCommandIndices[in_InstanceId]]
