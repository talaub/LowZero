#include "vertex_outputs.glsl"

#include "object.glsl"

#define WRITE_VIEW_POSITION(VPOS) o_ViewPosition = (g_ViewInfo.viewMatrix * vec4(VPOS, 1.0f)).xyz

