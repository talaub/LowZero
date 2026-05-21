#pragma once

#include "LowEditorWidget.h"

#include <imgui.h>

#include <memory.h>
#include <string>

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
      void init_editor();
      void enforce_standard_mode();
      void render_toolbar();
      void save_active_buffer();
      void reload_file();

      std::unique_ptr<Zep::ZepEditor_ImGui> m_Zep;
      std::string m_FilePath;
      std::string m_StatusText;
    };
  } // namespace Editor
} // namespace Low
