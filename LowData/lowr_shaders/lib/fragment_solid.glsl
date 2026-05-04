#include "object.glsl"

//shader input
#include "fragment_inputs.glsl"

//output write
#ifdef HIGHLIGHT
layout (location = 0) out vec4 o_Albedo;
layout (location = 1) out vec4 o_Normals;
layout (location = 2) out vec4 o_ViewPosition;
layout (location = 3) out uint o_ObjectId;
#else
layout (location = 0) out uint o_Highlight;
#endif
