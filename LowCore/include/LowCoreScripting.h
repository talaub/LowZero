#pragma once

#include "LowCoreApi.h"

#include "LowCoreScriptModule.h"
#include "LowCoreScriptClassInstance.h"
#include <vulkan/vulkan_core.h>
#include "LowCoreScriptAsset.h"

namespace Low {
  namespace Core {
    namespace Scripting {
      enum class EventType
      {
        Error,
        Info,
        Warn
      };

      struct EventMessage
      {
        EventType type;
        int col;
        int row;
        Util::String msg;
        ScriptAsset script;
      };

      void LOW_CORE_API initialize_as();
      void LOW_CORE_API cleanup_as();
      void LOW_CORE_API test_as();

      void LOW_CORE_API tick_as(const float p_Delta);

      void LOW_CORE_API build_module(Module p_Module);
      bool LOW_CORE_API fill_member_fields(ClassInstance p_Instance);
    } // namespace Scripting
  } // namespace Core
} // namespace Low
