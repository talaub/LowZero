#pragma once

#include "LowUtilContainers.h"

#include "LowMath.h"

namespace Low {
  namespace Editor {
    namespace ResourceProcessor {
      namespace Mesh {
        bool process(Util::String p_FilePath, Util::String p_OutputPath);
        void process_animations(Util::String p_FilePath,
                                Util::String p_OutputPath);
      } // namespace Mesh
    }   // namespace ResourceProcessor
  }     // namespace Editor
} // namespace Low
