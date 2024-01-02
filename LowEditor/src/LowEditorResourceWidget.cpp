#include "LowEditorResourceWidget.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome5.h"

namespace Low {
  namespace Editor {

    void ResourceWidget::render(float p_Delta)
    {
      ImGui::Begin(ICON_FA_ARCHIVE " Resources");
      ImGui::Text("Test");
      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
