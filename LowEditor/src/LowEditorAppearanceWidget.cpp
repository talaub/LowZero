#include "LowEditorAppearanceWidget.h"

#include <imgui.h>
#include "IconsLucide.h"
#include "LowEditorIcons.h"

namespace Low {
  namespace Editor {
    void AppearanceWidget::render(float p_Delta)
    {
      ImGui::Begin(ICON_LC_PALETTE " Appearance");
      ImGui::Text("Test");
      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
