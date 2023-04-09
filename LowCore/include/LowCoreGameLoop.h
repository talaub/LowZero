#pragma once

#include "LowCoreApi.h"
#include "LowCoreSystem.h"

#include "LowMath.h"

namespace Low {
  namespace Core {
    namespace GameLoop {
      void LOW_CORE_API initialize();
      void LOW_CORE_API start();
      void LOW_CORE_API stop();
      void LOW_CORE_API cleanup();

      uint32_t LOW_CORE_API get_fps();

      void LOW_CORE_API register_tick_callback(System::TickCallback p_Callback);
    } // namespace GameLoop
  }   // namespace Core
} // namespace Low
