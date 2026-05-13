#version 450

layout(location = 0) out vec4 fragColor;

#include "fullscreen_effect.glsl"

void main() {
    vec2 uv = gl_FragCoord.xy * g_ViewInfo.dimensions.zw;
    vec3 indirect = texture(TEX2D(g_ViewInfo.textureIndices.w), uv).rgb;
    fragColor = vec4(indirect, 1.0);
}
