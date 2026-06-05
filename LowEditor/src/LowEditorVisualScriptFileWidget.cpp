#include "LowEditorVisualScriptFileWidget.h"

#include "imgui.h"

namespace Low {
  namespace Editor {
    VisualScriptFileWidget::VisualScriptFileWidget(
        const Util::String &p_Path)
        : m_Path(p_Path)
    {
      m_Document.load_from_path(
          p_Path,
          VisualScript::get_context_registry(),
          VisualScript::get_compile_profile_registry());

      m_Editor.load_document(m_Document);
      m_Editor.embedded = false;
      m_Editor.sidebar_left = true;

      m_Title = p_Path;
      const size_t l_Slash = m_Title.find_last_of("/\\");
      if (l_Slash != Util::String::npos) {
        m_Title = m_Title.substr(l_Slash + 1);
      }
      m_Title += "##vs_file_";
      m_Title += p_Path;
    }

    void VisualScriptFileWidget::render(float p_Delta)
    {
      ImGui::SetNextWindowSize(ImVec2(1200.0f, 800.0f),
                               ImGuiCond_FirstUseEver);
      ImGui::Begin(m_Title.c_str(), &m_Open,
                   ImGuiWindowFlags_NoSavedSettings);
      m_Editor.render(p_Delta);
      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
