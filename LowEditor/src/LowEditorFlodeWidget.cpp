#include "LowEditorFlodeWidget.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"

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
      ImGui::Begin(ICON_FA_PROJECT_DIAGRAM " Flode");

      m_Editor->render(p_Delta);

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
