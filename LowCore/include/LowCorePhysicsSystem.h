#pragma once

#include "LowCoreApi.h"

#include "LowUtilEnums.h"

namespace Low {
  namespace Core {
    namespace System {
      namespace Physics {
        LOW_CORE_API void tick(float p_Delta,
                               Util::EngineState p_State);
        LOW_CORE_API void late_tick(float p_Delta,
                                    Util::EngineState p_State);
      } // namespace Physics
    } // namespace System
  } // namespace Core
} // namespace Low
