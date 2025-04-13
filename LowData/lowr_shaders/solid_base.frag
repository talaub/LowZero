#version 450

//shader input
layout(location = 0) flat in uint in_InstanceId;
layout(location = 1) in vec2 in_TextureCoordinates;
layout(location = 2) in vec3 in_SurfaceNormal;
layout(location = 3) in mat3 in_TBN;

//output write
layout (location = 0) out vec4 o_Albedo;
layout (location = 1) out vec4 o_Normals;

void main() 
{
	o_Albedo = vec4(0.0, 1.0, 1.0,1.0);

  vec3 l_SurfaceNormal = in_SurfaceNormal;

  o_Normals = vec4(vec3((in_SurfaceNormal.x + 1.0) / 2.0,
                              (in_SurfaceNormal.y + 1.0) / 2.0,
                              (in_SurfaceNormal.z + 1.0) / 2.0),
                         1.0);
}
