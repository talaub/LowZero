#include "LowEditorDetailsWidget.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"

namespace Low {
  namespace Editor {
    void DetailsWidget::render(float p_Delta)
    {
      ImGui::Begin("Details");

      for (auto it = m_Sections.begin(); it != m_Sections.end(); ++it) {
        it->render(p_Delta);
      }

      ImGui::End();
    }

    void DetailsWidget::add_section(const Util::Handle p_Handle)
    {
      m_Sections.push_back(HandlePropertiesSection(p_Handle));
    }

    void DetailsWidget::clear()
    {
      m_Sections.clear();
    }
  } // namespace Editor
} // namespace Low
