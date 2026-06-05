#pragma once

#include "LowEditorWidget.h"
#include "LowEditorVisualScriptEditor.h"
#include "LowUtilString.h"

namespace Low {
  namespace Editor {
    struct LOW_EDITOR_API VisualScriptFileWidget : public Widget
    {
      VisualScriptFileWidget(const Util::String &p_Path);

      void render(float p_Delta) override;

      bool matches_path(const Util::String &p_Path) const
      {
        return m_Path == p_Path;
      }

    private:
      Util::String m_Path;
      Util::String m_Title;
      VisualScript::Document m_Document;
      VisualScript::Editor m_Editor;
    };
  } // namespace Editor
} // namespace Low
