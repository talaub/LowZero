#include "LowEditorSceneWidget.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "IconsLucide.h"

#include "LowEditor.h"
#include "LowEditorMainWindow.h"
#include "LowEditorGui.h"
#include "LowEditorEntityTree.h"
#include "LowEditorIcons.h"

#include "LowCoreEntity.h"
#include "LowCorePrefab.h"
#include "LowCoreTransform.h"
#include "LowCoreMeshRenderer.h"
#include "LowCorePointLight.h"

#include "LowUtilString.h"

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
      (void)p_Delta;

      ImGui::Begin(ICON_LC_LIST_TREE " Scene");

      ImVec2 l_Cursor = ImGui::GetCursorScreenPos();
      ImRect l_Rect(l_Cursor,
                    {l_Cursor.x + ImGui::GetWindowWidth(),
                     l_Cursor.y + ImGui::GetWindowHeight()});

      if (ImGui::BeginDragDropTargetCustom(l_Rect, 89023)) {
        if (const ImGuiPayload *l_Payload =
                ImGui::AcceptDragDropPayload("DG_HANDLE")) {
          Core::Entity l_Entity = *(uint64_t *)l_Payload->Data;
          Core::Prefab l_Prefab = *(uint64_t *)l_Payload->Data;
          if (l_Entity.is_alive()) {
            EntityTree::detach_from_parent_preserve_world(l_Entity);
          } else if (l_Prefab.is_alive()) {
            Core::Region l_Region = get_region_for_new_entity();
            Core::Entity l_Instance = l_Prefab.spawn(l_Region);
            l_Instance.get_transform().position(Math::Vector3(0.0f));
            l_Instance.get_transform().rotation(Math::Quaternion());

            set_selected_entity(l_Instance);
          }
        }
        ImGui::EndDragDropTarget();
      }

      if (ImGui::IsWindowFocused()) {
        set_focused_widget(this);
      }

      if (ImGui::BeginPopup("create_entity_selection_popup")) {
        if (ImGui::Selectable(LOW_EDITOR_ICON_CYLINDER " Empty")) {
          Core::Entity l_Entity = Core::Entity::make("Empty");
          Core::Component::Transform::make(l_Entity);
          l_Entity.set_region(Util::find_handle_by_unique_id(
                                  *Core::Scene::get_loaded_scene()
                                       .get_regions()
                                       .begin())
                                  .get_id());
          set_selected_entity(l_Entity);
        }
        ImGui::Separator();
        if (ImGui::Selectable(LOW_EDITOR_ICON_CUBE
                              " Mesh renderer")) {
          Core::Entity l_Entity = Core::Entity::make("Mesh");
          Core::Component::Transform::make(l_Entity);
          Core::Component::MeshRenderer::make(l_Entity);
          l_Entity.set_region(Util::find_handle_by_unique_id(
                                  *Core::Scene::get_loaded_scene()
                                       .get_regions()
                                       .begin())
                                  .get_id());
          set_selected_entity(l_Entity);
        }
        if (ImGui::Selectable(LOW_EDITOR_ICON_POINT_LIGHT
                              " Point light")) {
          Core::Entity l_Entity = Core::Entity::make("PointLight");
          Core::Component::Transform::make(l_Entity);
          Core::Component::PointLight l_PointLight =
              Core::Component::PointLight::make(l_Entity);
          l_PointLight.set_color(
              Low::Math::ColorRGB(1.0f, 1.0f, 1.0f));
          l_PointLight.set_intensity(1.0f);
          l_Entity.set_region(Util::find_handle_by_unique_id(
                                  *Core::Scene::get_loaded_scene()
                                       .get_regions()
                                       .begin())
                                  .get_id());
          set_selected_entity(l_Entity);
        }
        ImGui::EndPopup();
      }

      if (Gui::AddButton("Create")) {
        ImGui::OpenPopup("create_entity_selection_popup");
      }

      ImGui::SameLine();

      static char l_Search[128] = "";
      Gui::SearchField("##searchinput", l_Search,
                       IM_ARRAYSIZE(l_Search), {0.0f, 2.0f});

      ImGui::Dummy({0.0f, 4.0f});

      Low::Util::String l_SearchString = l_Search;
      l_SearchString.make_lower();

      bool l_OpenedEntryPopup = false;

      for (auto it = Core::Entity::ms_LivingInstances.begin();
           it != Core::Entity::ms_LivingInstances.end(); ++it) {
        if (Core::Component::Transform(
                it->get_transform().get_parent())
                .is_alive()) {
          continue;
        }
        if (!EntityTree::matches_search(*it, l_SearchString)) {
          continue;
        }
        if (EntityTree::render(*it, &l_OpenedEntryPopup,
                               l_SearchString)) {
          break;
        }
      }

      if (!l_OpenedEntryPopup) {
        if (ImGui::BeginPopupContextWindow("WINDOWCONTEXT")) {
          if (ImGui::MenuItem("New entity")) {
            Core::Region l_Region = get_region_for_new_entity();

            if (l_Region.is_alive()) {
              Core::Entity l_Entity =
                  Core::Entity::make(N(NewEntity), l_Region);
              Core::Component::Transform::make(l_Entity);

              get_global_changelist().add_entry(
                  History::create_handle_transaction(l_Entity));

              set_selected_entity(l_Entity);
            } else {
              LOW_LOG_ERROR
                  << "Could not create new entity. There is no "
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
