#pragma once

#include "LowUtilApi.h"

#include "LowUtilContainers.h"

#include "LowMath.h"

namespace Low {
  namespace Util {
    namespace Resource {
      struct Image2D
      {
        List<List<uint8_t>> data;
        List<Math::UVector2> dimensions;
      };

      struct Vertex
      {
        Math::Vector3 position;
      };

      struct Mesh
      {
        List<Vertex> vertices;
        List<uint32_t> indices;
      };

      LOW_EXPORT void load_image2d(String p_FilePath, Image2D &p_Image);

      LOW_EXPORT void load_mesh(String p_FilePath, Mesh &p_Mesh);
    } // namespace Resource
  }   // namespace Util
} // namespace Low
