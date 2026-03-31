layout(location = 0) out uint o_InstanceId;
layout(location = 1) out vec2 o_TextureCoordinates;

#include "vertex_buffer.glsl"

#include "ui.glsl"

#define UI_DRAW_COMMAND g_UiDrawCommands[g_UiDrawCommandIndices[gl_InstanceIndex]]
