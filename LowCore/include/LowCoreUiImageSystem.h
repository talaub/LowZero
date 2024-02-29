#pragma once

#include "LowUtilEnums.h"

#include "LowRenderer.h"

namespace Low {
  namespace Core {
    namespace UI {
      namespace System {
        namespace Image {
          void tick(float p_Delta, Util::EngineState p_State);

          Renderer::Mesh get_mesh();
        } // namespace Image
      }   // namespace System
    }     // namespace UI
  }       // namespace Core
} // namespace Low
