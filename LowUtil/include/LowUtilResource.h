#pragma once

#include "LowUtilApi.h"

#include "LowUtilContainers.h"

#include "LowMath.h"

namespace Low {
  namespace Util {
    namespace Resource {
      struct Image2D
      {
        List<uint8_t> data;
        Math::UVector2 dimensions;
      };

      LOW_EXPORT void load_dds(String p_FilePath, Image2D &p_Image);
    } // namespace Resource
  }   // namespace Util
} // namespace Low
