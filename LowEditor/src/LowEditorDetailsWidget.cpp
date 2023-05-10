#include "LowEditorDetailsWidget.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"

namespace Low {
  namespace Editor {
    void DetailsWidget::render(float p_Delta)
    {
      m_BreakRunning = false;

      ImGui::Begin(ICON_FA_GLASSES " Details");

      for (auto it = m_Sections.begin(); it != m_Sections.end(); ++it) {
        if (m_BreakRunning) {
          break;
        }
        it->render(p_Delta);
      }

      ImGui::End();
    }

    void DetailsWidget::add_section(const Util::Handle p_Handle)
    {
      Util::RTTI::TypeInfo &l_TypeInfo =
          Util::Handle::get_type_info(p_Handle.get_type());

      if (!l_TypeInfo.is_alive(p_Handle)) {
        return;
      }

      HandlePropertiesSection l_Section(p_Handle, false);
      l_Section.render_footer = nullptr;

      add_section(l_Section);
    }

    void DetailsWidget::add_section(HandlePropertiesSection p_Section)
    {
      m_Sections.push_back(p_Section);
      m_BreakRunning = true;
    }

    void DetailsWidget::clear()
    {
      m_Sections.clear();
      m_BreakRunning = true;
    }
  } // namespace Editor
} // namespace Low
