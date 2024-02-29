#pragma once

#include "LowUtilEnums.h"

#include "LowRenderer.h"

namespace Low {
  namespace Core {
    namespace UI {
      namespace System {
        namespace Text {
          void tick(float p_Delta, Util::EngineState p_State);

          Renderer::Material get_material();
        } // namespace Text
      }   // namespace System
    }     // namespace UI
  }       // namespace Core
} // namespace Low
