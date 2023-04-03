#include "LowEditorHandlePropertiesSection.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"

#include "LowEditorPropertyEditors.h"

namespace Low {
  namespace Editor {
    void HandlePropertiesSection::render(float p_Delta)
    {
      if (!m_TypeInfo.is_alive(m_Handle)) {
        return;
      }

      if (ImGui::CollapsingHeader(m_TypeInfo.name.c_str())) {
        ImGui::PushID(m_Handle.get_id());
        for (auto pit = m_TypeInfo.properties.begin();
             pit != m_TypeInfo.properties.end(); ++pit) {

          PropertyEditors::render_editor(pit->second,
                                         pit->second.get(m_Handle));
        }
        ImGui::PopID();
      }
    }

    HandlePropertiesSection::HandlePropertiesSection(
        const Util::Handle p_Handle)
        : m_Handle(p_Handle)
    {
      m_TypeInfo = Util::Handle::get_type_info(p_Handle.get_type());
    }
  } // namespace Editor
} // namespace Low
