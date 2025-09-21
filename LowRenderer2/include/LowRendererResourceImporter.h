#pragma once

// This is supposed to handle the offline import of files

#include "LowRenderer2Api.h"

#include "LowUtilContainers.h"

namespace Low {
  namespace Renderer {
    namespace ResourceImporter {

      bool LOW_RENDERER2_API import_mesh(Util::String p_ImportPath,
                                         Util::String p_OutputPath);
      bool LOW_RENDERER2_API import_font(Util::String p_ImportPath,
                                         Util::String p_OutputPath);
      bool LOW_RENDERER2_API import_texture(
          Util::String p_ImportPath, Util::String p_OutputPath);
    } // namespace ResourceImporter
  } // namespace Renderer
} // namespace Low
