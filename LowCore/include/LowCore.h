#pragma once

#include "LowCoreApi.h"

#include "LowUtilEnums.h"

namespace Low {
  namespace Core {
    LOW_CORE_API void initialize();
    LOW_CORE_API void cleanup();

    LOW_CORE_API Util::EngineState get_engine_state();
    LOW_CORE_API void begin_playmode();
    LOW_CORE_API void exit_playmode();
  } // namespace Core
} // namespace Low
