#include "LowEditorSceneWidget.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"

#include "LowEditorMainWindow.h"

#include "LowCoreEntity.h"

namespace Low {
  namespace Editor {
    void SceneWidget::render(float p_Delta)
    {
      ImGui::Begin(ICON_FA_LIST_UL " Scene");

      for (auto it = Core::Entity::ms_LivingInstances.begin();
           it != Core::Entity::ms_LivingInstances.end(); ++it) {
        if (ImGui::Selectable(it->get_name().c_str(),
                              it->get_id() == get_selected_entity().get_id())) {
          set_selected_entity(*it);
        }
      }

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
