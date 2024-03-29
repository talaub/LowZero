#pragma once

#include "LowCoreApi.h"

#include "CflatGlobal.h"
#include "Cflat.h"

#include "LowUtilEnums.h"

namespace Low {
  namespace Core {
    namespace Scripting {
      void initialize();
      void tick(float p_Delta, Util::EngineState p_State);
      void cleanup();
      LOW_CORE_API Cflat::Environment *get_environment();
    } // namespace Scripting
  }   // namespace Core
} // namespace Low
