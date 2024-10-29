#include "LowEditorScriptingErrorWidget.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"

namespace Low {
  namespace Editor {
    void ScriptingErrorWidget::render(float p_Delta)
    {
      ImGui::SetNextWindowSize(ImVec2(500, 400),
                               ImGuiCond_FirstUseEver);

      ImGui::Begin(ICON_FA_BUG " Scripting errors", &m_Open);

      m_ErrorList.render(p_Delta);
      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
