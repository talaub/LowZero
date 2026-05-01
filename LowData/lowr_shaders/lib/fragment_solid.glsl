#include "object.glsl"

//shader input
layout(location = 0) flat in uint in_InstanceId;
layout(location = 1) in vec2 in_TextureCoordinates;
layout(location = 2) in vec3 in_SurfaceNormal;
layout(location = 3) in vec3 in_ViewPosition;
layout(location = 4) in mat3 in_TBN;

//output write
layout (location = 0) out vec4 o_Albedo;
layout (location = 1) out vec4 o_Normals;
layout (location = 2) out vec4 o_ViewPosition;
layout (location = 3) out uint o_ObjectId;
