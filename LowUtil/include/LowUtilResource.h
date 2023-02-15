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

      LOW_EXPORT void load_image2d(String p_FilePath, Image2D &p_Image);
    } // namespace Resource
  }   // namespace Util
} // namespace Low
