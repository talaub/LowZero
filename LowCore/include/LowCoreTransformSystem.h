#pragma once

#include "LowUtilEnums.h"

namespace Low {
  namespace Core {
    namespace System {
      namespace Transform {
        void tick(float p_Delta, Util::EngineState p_State);
        void late_tick(float p_Delta, Util::EngineState p_State);
      } // namespace Transform
    }   // namespace System
  }     // namespace Core
} // namespace Low
