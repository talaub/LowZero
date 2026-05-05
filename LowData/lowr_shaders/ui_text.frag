#version 450

#include "ui_fragment.glsl"

#extension GL_EXT_nonuniform_qualifier : enable

float median3(vec3 v)
{
    // median without branches
    return max(min(v.r, v.g), min(max(v.r, v.g), v.b));
}

void main()
{
    vec2 min_uv = UI_DRAW_COMMAND.uvRect.xy;
    vec2 max_uv = UI_DRAW_COMMAND.uvRect.zw;

    vec2 l_uv = mix(min_uv, max_uv, in_TextureCoordinates);

    // Sample MSDF using bindless index; needs nonuniformEXT
    vec3 l_msdf = texture(TEX2D(nonuniformEXT(UI_DRAW_COMMAND.textureIndex)), l_uv).rgb;

    // Decode MSDF (median of RGB channels)
    float l_m = median3(l_msdf);

    // Screen-space width for sharp, resolution-independent edges
    float l_w = fwidth(l_m);

    float edge = 0.0;
    float softness = 1.0;

    // Threshold center with thickness bias (edge), and adjustable softness
    float l_center = 0.5 + edge;
    float l_alpha = smoothstep(l_center - l_w * softness,
            l_center + l_w * softness,
            l_m);

    float l_finalAlpha = l_alpha * UI_DRAW_COMMAND.color.a;
    o_Color = vec4(UI_DRAW_COMMAND.color.rgb, l_finalAlpha);
  }
