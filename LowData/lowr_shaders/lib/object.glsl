#include "vertex_buffer.glsl"

#include "globals.glsl"
#include "viewinfo.glsl"

struct RenderObject {
  mat4 worldMatrix;
  int materialIndex;
  int objectId;
};

layout(std140, set = 0, binding = 1) readonly buffer RenderObjectWrapper
{
  RenderObject g_RenderObjects[];
};

layout(push_constant) uniform constants
{
    uint renderObjectSlot;
} c_RenderEntry;

#define RENDER_OBJECT g_RenderObjects[c_RenderEntry.renderObjectSlot]
