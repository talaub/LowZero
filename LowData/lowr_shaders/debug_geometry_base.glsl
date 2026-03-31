#include "vertex_buffer.glsl"

#include "globals.glsl"
#include "viewinfo.glsl"

struct DebugGeometryRenderObject {
    mat4 worldMatrix;
    vec4 color;
    uint editorImageIndex;
};

layout(std140, set = 2, binding = 5) readonly buffer DebugGeometryRenderObjectWrapper
{
    DebugGeometryRenderObject g_DebugGeometryRenderObjects[];
};

layout(push_constant) uniform constants
{
    uint renderObjectSlot;
} c_RenderEntry;

#define RENDER_OBJECT g_DebugGeometryRenderObjects[c_RenderEntry.renderObjectSlot]
