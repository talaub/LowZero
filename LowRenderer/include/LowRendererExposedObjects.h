#pragma once

#include "LowRendererMesh.h"
#include "LowRendererSkeleton.h"
#include "LowRendererSkeletalAnimation.h"
#include "LowRendererMaterial.h"
#include "LowRendererTexture2D.h"

#include "LowMath.h"

namespace Low {
  namespace Renderer {
    struct RenderObject
    {
      Math::Matrix4x4 transform;

      Mesh mesh;
      Material material;

      Math::Color color;

      uint64_t entity_id;

      bool useSkinningBuffer = false;
      uint32_t vertexBufferStartOverride = 0;
    };
  } // namespace Renderer
} // namespace Low
