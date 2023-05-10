#include "LowEditorSceneWidget.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"

#include "LowEditorMainWindow.h"

#include "LowCoreEntity.h"
#include "LowCoreTransform.h"

namespace Low {
  namespace Editor {
    void SceneWidget::render(float p_Delta)
    {
      ImGui::Begin(ICON_FA_LIST_UL " Scene");

      bool l_OpenedEntryPopup = false;

      for (auto it = Core::Entity::ms_LivingInstances.begin();
           it != Core::Entity::ms_LivingInstances.end(); ++it) {
        bool i_Break = false;
        if (ImGui::Selectable(it->get_name().c_str(),
                              it->get_id() == get_selected_entity().get_id())) {
          set_selected_entity(*it);
        }
        if (ImGui::BeginPopupContextItem()) {
          l_OpenedEntryPopup = true;
          set_selected_entity(*it);
          if (ImGui::MenuItem("Delete")) {
            Core::Entity::destroy(*it);
            i_Break = true;
          }
          ImGui::EndPopup();
        }

        if (i_Break) {
          break;
        }
      }

      if (!l_OpenedEntryPopup) {
        if (ImGui::BeginPopupContextWindow()) {
          if (ImGui::MenuItem("New entity")) {
            Core::Entity l_Entity = Core::Entity::make(N(NewEntity));
            Core::Component::Transform::make(l_Entity);

            set_selected_entity(l_Entity);
          }
          ImGui::EndPopup();
        }
      }

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
