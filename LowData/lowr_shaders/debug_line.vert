#version 450

#include "viewinfo.glsl"

layout(location = 0) out vec4 out_Color;

layout(push_constant) uniform constants
{
    vec4 start;
    vec4 end;
    vec4 color;
} c_Line;

void main()
{
    vec3 l_Position = gl_VertexIndex == 0 ? c_Line.start.xyz : c_Line.end.xyz;
    gl_Position = g_ViewInfo.viewProjectionMatrix * vec4(l_Position, 1.0);
    out_Color = c_Line.color;
}
