#include "vertex_buffer.glsl"

#ifndef SHADOW
#include "globals.glsl"
#include "viewinfo.glsl"
#endif

struct RenderObject {
  mat4 worldMatrix;
  int materialIndex;
  int objectId;
};

layout(std140, set = 0, binding = 1) readonly buffer RenderObjectWrapper
{
  RenderObject g_RenderObjects[];
};

#if defined(SHADOW)
layout(push_constant) uniform constants
{
    uint renderObjectSlot;
    uint _pad0;
    uint _pad1;
    uint _pad2;
    mat4 lightSpaceMatrix;
} c_RenderEntry;
#elif defined(HIGHLIGHT)
layout(push_constant) uniform constants
{
    uint renderObjectSlot;
    uint highlight;
} c_RenderEntry;
#else
layout(push_constant) uniform constants
{
    uint renderObjectSlot;
} c_RenderEntry;
#endif

#define RENDER_OBJECT g_RenderObjects[c_RenderEntry.renderObjectSlot]
