#include "LowEditorFlodeWidget.h"

#include "imgui.h"
#include "IconsLucide.h"

#include "LowEditor.h"
#include "LowEditorMainWindow.h"
#include "LowEditorDetailsWidget.h"
#include "LowEditorHandlePropertiesSection.h"
#include "LowEditorThemes.h"

#include "LowUtilLogger.h"

#include "utilities/builders.h"
#include "utilities/drawing.h"
#include "utilities/widgets.h"

namespace Low {
  namespace Editor {
    FlodeWidget::FlodeWidget()
    {
      m_Editor = new Flode::Editor();
    }

    void FlodeWidget::render(float p_Delta)
    {
      ImGui::Begin(ICON_LC_WORKFLOW " Flode");
      if (ImGui::IsWindowFocused()) {
        set_focused_widget(this);
      }

      m_Editor->render(p_Delta);

      ImGui::End();
    }

    bool FlodeWidget::handle_shortcuts(float p_Delta)
    {
      return true;
    }
  } // namespace Editor
} // namespace Low
