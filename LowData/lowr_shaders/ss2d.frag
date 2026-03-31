#version 450

#define DRAW_COMMAND_COUNT 100
#define DRAW_COMMAND_CUTOFF 1000

layout(location = 0) out vec4 outColor;

struct DrawCommand {
  vec4 color;
  vec4 corners;
  vec2 position;
  vec2 half_extents;
  uint type;
};

layout(std140, set = 0, binding = 0) readonly buffer DrawCommandWrapper
{
  DrawCommand g_DrawCommands[];
};

layout(std430, set = 1, binding = 0) readonly buffer DrawCommandIndicesWrapper
{
  uint g_DrawCommandIndices[];
};

float sdBox(vec2 p, vec2 b)
{
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float sdCircle(vec2 p, float r)
{
    return length(p) - r;
}

float sdRoundBox4(vec2 p, vec2 b, vec4 r)
{
    float radius;

    if (p.x >= 0.0)
        radius = (p.y >= 0.0) ? r.y : r.z;
    else
        radius = (p.y >= 0.0) ? r.x : r.w;

    vec2 q = abs(p) - b + vec2(radius);
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - radius;
}

void main()
{

  vec4 l_OutColor = vec4(0.0, 0.0, 0.0, 0.0);

  for (uint i = 0; i < DRAW_COMMAND_COUNT; ++i){
    uint i_DrawCommandIndex = g_DrawCommandIndices[i];
    if (i_DrawCommandIndex > DRAW_COMMAND_CUTOFF){
      continue;
    }

    DrawCommand i_DrawCommand = g_DrawCommands[i_DrawCommandIndex];

    vec2 p = gl_FragCoord.xy - i_DrawCommand.position;

    float d = 1.0f;
    if (i_DrawCommand.type == 0){
    d = sdCircle(p, i_DrawCommand.half_extents.x);
    }
    else if (i_DrawCommand.type == 1){
    d = sdBox(p, i_DrawCommand.half_extents);
    }
    else if (i_DrawCommand.type == 2){
    d = sdRoundBox4(p, i_DrawCommand.half_extents, i_DrawCommand.corners);
    }

    float aa = max(fwidth(d), 1e-4);
    float alpha = 1.0 - smoothstep(0.0, aa, d);

    if (alpha > 0.02){
      l_OutColor += vec4(i_DrawCommand.color.rgb, i_DrawCommand.color.a * alpha);
    }

  }

  outColor = l_OutColor;
}
