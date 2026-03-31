struct ViewInfo {
  mat4 viewMatrix;
  mat4 inverseViewMatrix;
  mat4 inverseProjectionMatrix;
  mat4 projectionMatrix;
  mat4 viewProjectionMatrix;
  mat4 uiViewProjectionMatrix;
  uvec4 gbufferIndices;
  uvec4 textureIndices;
  uvec4 lightClusters;
  vec4 dimensions;
  vec2 nearFarPlane;
};

layout(std140, set = 2, binding = 0) uniform ViewInfoBuffer{
  ViewInfo g_ViewInfo;
};
