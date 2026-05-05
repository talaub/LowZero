#version 450

#include "globals.glsl"

layout(location = 0) out vec4 o_Color;

layout(push_constant) uniform PushConstants {
    vec2 texelSize;
    uint highlightMapIndex;
} pc;

#define HIGHLIGHT_NONE     0xFFFFFFFFu
#define HIGHLIGHT_SELECTED 0u
#define HIGHLIGHT_HOVERED  1u

const vec4 COLOR_SELECTED = vec4(1.0, 0.7, 0.0, 1.0);
const vec4 COLOR_HOVERED  = vec4(0.3, 0.7, 1.0, 1.0);

uint sample_highlight(vec2 uv) {
    return texture(usampler2D(g_UTexture2Ds[pc.highlightMapIndex], g_Samplers[0]), uv).r;
}

void main() {
    vec2 uv = gl_FragCoord.xy * pc.texelSize;

    uint center = sample_highlight(uv);
    if (center == HIGHLIGHT_NONE) {
        discard;
    }

    uint n = sample_highlight(uv + vec2(0.0,  pc.texelSize.y));
    uint s = sample_highlight(uv + vec2(0.0, -pc.texelSize.y));
    uint e = sample_highlight(uv + vec2( pc.texelSize.x, 0.0));
    uint w = sample_highlight(uv + vec2(-pc.texelSize.x, 0.0));

    if (n == center && s == center && e == center && w == center) {
        discard;
    }

    o_Color = (center == HIGHLIGHT_SELECTED) ? COLOR_SELECTED : COLOR_HOVERED;
}
