#pragma once

#include "LowCoreCflatScripting.h"

#include "LowUtilContainers.h"

namespace Low {
  namespace Editor {
    struct ScriptingErrorList
    {
      ScriptingErrorList();
      void render(float p_Delta);

      void error_callback(const Core::Scripting::Error &p_Error);
      void compilation_callback(
          const Core::Scripting::Compilation &p_Compilation);

    private:
      Util::List<Core::Scripting::Error> m_Errors;
    };
  } // namespace Editor
} // namespace Low
