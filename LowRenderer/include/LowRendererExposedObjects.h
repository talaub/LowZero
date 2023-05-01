#pragma once

#include "LowRendererMesh.h"
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
    };
  } // namespace Renderer
} // namespace Low
