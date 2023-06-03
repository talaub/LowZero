#include "LowEditorSceneWidget.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"

#include "LowEditorMainWindow.h"

#include "LowCoreEntity.h"
#include "LowCoreTransform.h"

namespace Low {
  namespace Editor {
    static Core::Region get_region_for_new_entity()
    {
      Core::Region l_Result;
      for (auto it = Core::Region::ms_LivingInstances.begin();
           it != Core::Region::ms_LivingInstances.end(); ++it) {
        if (!it->get_scene().is_loaded()) {
          continue;
        }
        if (!it->is_streaming_enabled() && it->is_loaded()) {
          return *it;
        }
        if (it->is_streaming_enabled()) {
          if (!l_Result.is_alive()) {
            l_Result = *it;
          }
          if (it->is_loaded() && !l_Result.is_loaded()) {
            l_Result = *it;
          }
        }
      }

      return l_Result;
    }

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
            Core::Region l_Region = get_region_for_new_entity();

            if (l_Region.is_alive()) {
              Core::Entity l_Entity =
                  Core::Entity::make(N(NewEntity), l_Region);
              Core::Component::Transform::make(l_Entity);

              set_selected_entity(l_Entity);
            } else {
              LOW_LOG_ERROR << "Could not create new entity. There is no "
                               "suitable region in this scene."
                            << LOW_LOG_END;
            }
          }
          ImGui::EndPopup();
        }
      }

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
