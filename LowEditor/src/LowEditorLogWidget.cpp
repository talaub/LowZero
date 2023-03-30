#include "LowEditorLogWidget.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"

namespace Low {
  namespace Editor {
    void LogWidget::render(float p_Delta)
    {
      ImGui::Begin(ICON_FA_SCROLL " Log");
      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
