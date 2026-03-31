#include "globals.glsl"
#include "viewinfo.glsl"

struct UiDrawCommand{
    vec4 uvRect;
    vec4 color;
    vec2 position;
    vec2 size;
    float rotation2D;
    uint textureIndex;
    uint materialIndex;
};

layout(std430, set = 2, binding = 4) readonly buffer UiDrawCommandIndicesWrapper
{
  uint g_UiDrawCommandIndices[];
};

layout(std140, set = 0, binding = 3) readonly buffer UiDrawCommandWrapper
{
  UiDrawCommand g_UiDrawCommands[];
};
