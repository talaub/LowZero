#pragma once

#include "LowEditorWidget.h"

#include <imgui.h>

#include <memory.h>

namespace Zep {
  class ZepEditor_ImGui;
}

namespace Low {
  namespace Editor {
    struct ScriptWidget : public Widget
    {
      ScriptWidget();

      void render(float p_Delta) override;

      void load_file(Util::String p_Path);

    private:
      std::unique_ptr<Zep::ZepEditor_ImGui> m_Zep;
    };
  } // namespace Editor
} // namespace Low
