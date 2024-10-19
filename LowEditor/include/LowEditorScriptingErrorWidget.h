#pragma once

#include "LowEditorWidget.h"
#include "LowEditorScriptingErrorList.h"

namespace Low {
  namespace Editor {
    struct ScriptingErrorWidget : public Widget
    {
      void render(float p_Delta) override;

    private:
      ScriptingErrorList m_ErrorList;
    };
  } // namespace Editor
} // namespace Low
