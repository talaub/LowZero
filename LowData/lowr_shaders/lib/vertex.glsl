layout(location = 0) out uint o_InstanceId;
layout(location = 1) out vec2 o_TextureCoordinates;
layout(location = 2) out vec3 o_SurfaceNormal;
layout(location = 3) out vec3 o_ViewPosition;
layout(location = 4) out mat3 o_TBN;

#include "object.glsl"

#define WRITE_VIEW_POSITION(VPOS) o_ViewPosition = (g_ViewInfo.viewMatrix * vec4(VPOS, 1.0f)).xyz

