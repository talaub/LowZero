#version 450

vec2 positions[3] = vec2[](vec2(-1.0, -2.0), vec2(2.0, 2.0), vec2(-1.0, 2.0));

void main()
{
  vec2 outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  gl_Position = vec4(outUV * 2.0f + -1.0f, 0.0f, 1.0f);
}
