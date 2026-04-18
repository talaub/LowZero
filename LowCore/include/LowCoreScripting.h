#pragma once

#include "LowCoreApi.h"

#include "LowCoreScriptModule.h"

namespace Low {
  namespace Core {
    namespace Scripting {
      void LOW_CORE_API initialize_as();
      void LOW_CORE_API cleanup_as();
      void LOW_CORE_API test_as();

      void LOW_CORE_API tick_as(const float p_Delta);

      void LOW_CORE_API build_module(Module p_Module);
    } // namespace Scripting
  } // namespace Core
} // namespace Low
