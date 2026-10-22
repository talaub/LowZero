#version 450

layout(location = 0) in vec4 in_Color;

layout(location = 0) out vec4 o_Color;

void main()
{
    o_Color = in_Color;
}
