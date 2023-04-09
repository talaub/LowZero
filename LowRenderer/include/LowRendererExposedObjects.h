#pragma once

#include "LowRendererMesh.h"
#include "LowRendererMaterial.h"

#include "LowMath.h"

namespace Low {
  namespace Renderer {
    struct RenderObject
    {
      Math::Vector3 world_position;
      Math::Quaternion world_rotation;
      Math::Vector3 world_scale;

      Mesh mesh;
      Material material;

      uint64_t entity_id;
    };
  } // namespace Renderer
} // namespace Low
