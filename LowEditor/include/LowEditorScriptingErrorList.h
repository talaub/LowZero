#pragma once

#include "LowCoreCflatScripting.h"

#include "LowCoreScriptModule.h"
#include "LowCoreScripting.h"
#include "LowUtilContainers.h"

namespace Low {
  namespace Editor {
    struct ScriptingErrorList
    {
      ScriptingErrorList();
      void render(float p_Delta);

      void msg_callback(const Core::Scripting::EventMessage &p_Msg);
      void compilation_callback(const Core::ScriptModule p_Module);

    private:
      Util::List<Core::Scripting::EventMessage> m_Messages;
    };
  } // namespace Editor
} // namespace Low
