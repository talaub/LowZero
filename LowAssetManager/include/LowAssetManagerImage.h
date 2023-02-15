#pragma once

#include "LowUtilContainers.h"

#include "LowMath.h"

namespace Low {
  namespace AssetManager {
    namespace Image {
      struct Image2D
      {
        Math::UVector2 dimensions;
        uint8_t channels;
        Util::List<uint8_t> data;
      };

      void load_png(Util::String p_FilePath, Image2D &p_Image);

      void process(Util::String p_OutputPath, Image2D &p_Image);
    } // namespace Image
  }   // namespace AssetManager
} // namespace Low
