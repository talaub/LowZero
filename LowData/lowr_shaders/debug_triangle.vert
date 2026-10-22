#version 450

#include "viewinfo.glsl"

layout(location = 0) out vec4 out_Color;

layout(push_constant) uniform constants
{
    vec4 p0;
    vec4 p1;
    vec4 p2;
    vec4 color;
} c_Triangle;

void main()
{
    vec3 l_Position = c_Triangle.p0.xyz;
    if (gl_VertexIndex == 1) {
        l_Position = c_Triangle.p1.xyz;
    } else if (gl_VertexIndex == 2) {
        l_Position = c_Triangle.p2.xyz;
    }

    gl_Position = g_ViewInfo.viewProjectionMatrix * vec4(l_Position, 1.0);
    out_Color = c_Triangle.color;
}
