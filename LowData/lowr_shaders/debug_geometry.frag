#version 450

#include "fragment_inputs.glsl"

layout(location = 0) out vec4 o_Color;

#include "debug_geometry_base.glsl"

void main()
{
    if (RENDER_OBJECT.editorImageIndex > 130) {
        o_Color = RENDER_OBJECT.color;
        return;
    }

    o_Color = texture(g_EditorImages[RENDER_OBJECT.editorImageIndex], in_TextureCoordinates);
}
