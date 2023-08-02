#include "LowEditorSceneWidget.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome5.h"

#include "LowEditorMainWindow.h"
#include "LowEditorGui.h"
#include "LowEditorCommonOperations.h"

#include "LowCoreEntity.h"
#include "LowCorePrefab.h"
#include "LowCoreTransform.h"

#include "LowUtilString.h"

#include "LowMathQuaternionUtil.h"
#include <glm/gtc/matrix_transform.hpp>

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

    static bool render_entity(Core::Entity p_Entity, bool *p_OpenedEntryPopup,
                              float p_Delta)
    {
      bool l_Break = false;

      if (ImGui::Selectable(p_Entity.get_name().c_str(),
                            p_Entity.get_id() ==
                                get_selected_entity().get_id())) {
        set_selected_entity(p_Entity);
      }
      if (ImGui::BeginPopupContextItem(
              LOW_TO_STRING(p_Entity.get_id()).c_str())) {
        *p_OpenedEntryPopup = true;
        set_selected_entity(p_Entity);
        if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
          duplicate({p_Entity});
          l_Break = true;
        }
        if (ImGui::MenuItem("Delete")) {
          get_global_changelist().add_entry(
              Helper::destroy_handle_transaction(p_Entity));
          Core::Entity::destroy(p_Entity);
          l_Break = true;
        }
        ImGui::EndPopup();
      }
      Gui::drag_handle(p_Entity);
      if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *l_Payload =
                ImGui::AcceptDragDropPayload("DG_HANDLE")) {
          Core::Entity l_Entity = *(uint64_t *)l_Payload->Data;
          if (l_Entity.is_alive()) {

            Math::Matrix4x4 l_InverseParentMatrix =
                glm::inverse(p_Entity.get_transform().get_world_matrix());

            Math::Matrix4x4 l_LocalMatrix =
                l_InverseParentMatrix *
                l_Entity.get_transform().get_world_matrix();

            glm::vec3 scale;
            glm::quat rotation;
            glm::vec3 translation;

            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(l_LocalMatrix, scale, rotation, translation, skew,
                           perspective);

            Transaction l_Transaction("Parent entity");
            l_Transaction.add_operation(
                new CommonOperations::PropertyEditOperation(
                    l_Entity.get_transform(), N(position),
                    l_Entity.get_transform().position(), translation));
            l_Transaction.add_operation(
                new CommonOperations::PropertyEditOperation(
                    l_Entity.get_transform(), N(rotation),
                    l_Entity.get_transform().rotation(), rotation));
            l_Transaction.add_operation(
                new CommonOperations::PropertyEditOperation(
                    l_Entity.get_transform(), N(scale),
                    l_Entity.get_transform().scale(), scale));
            l_Transaction.add_operation(
                new CommonOperations::PropertyEditOperation(
                    l_Entity.get_transform(), N(parent),
                    Util::Variant::from_handle(
                        l_Entity.get_transform().get_parent()),
                    Util::Variant::from_handle(p_Entity.get_transform())));

            get_global_changelist().add_entry(l_Transaction);

            l_Entity.get_transform().position(translation);
            l_Entity.get_transform().rotation(rotation);
            l_Entity.get_transform().scale(scale);

            l_Entity.get_transform().set_parent(
                p_Entity.get_transform().get_id());
          }
        }

        ImGui::EndDragDropTarget();
      }
      return l_Break;
    }

    static bool render_entity_hierarchy(Core::Entity p_Entity,
                                        bool *p_OpenedEntryPopup, float p_Delta)
    {
      bool l_Break = false;
      if (!p_Entity.get_transform().get_children().empty()) {
        Util::String l_IdString = "##";
        l_IdString += p_Entity.get_id();
        bool l_Open = ImGui::TreeNode(l_IdString.c_str());
        ImGui::SameLine();
        l_Break = render_entity(p_Entity, p_OpenedEntryPopup, p_Delta);

        if (l_Open) {
          for (auto it = p_Entity.get_transform().get_children().begin();
               it != p_Entity.get_transform().get_children().end(); ++it) {
            Core::Component::Transform i_Transform = *it;
            if (!i_Transform.is_alive()) {
              continue;
            }

            bool i_Break = render_entity_hierarchy(i_Transform.get_entity(),
                                                   p_OpenedEntryPopup, p_Delta);
            if (!l_Break) {
              l_Break = i_Break;
            }
          }
          ImGui::TreePop();
        }
      } else {
        l_Break = render_entity(p_Entity, p_OpenedEntryPopup, p_Delta);
      }

      return l_Break;
    }

    void SceneWidget::render(float p_Delta)
    {
      ImGui::Begin(ICON_FA_LIST_UL " Scene");

      ImVec2 l_Cursor = ImGui::GetCursorScreenPos();
      ImRect l_Rect(l_Cursor, {l_Cursor.x + ImGui::GetWindowWidth(),
                               l_Cursor.y + ImGui::GetWindowHeight()});

      if (ImGui::BeginDragDropTargetCustom(l_Rect, 89023)) {
        if (const ImGuiPayload *l_Payload =
                ImGui::AcceptDragDropPayload("DG_HANDLE")) {
          Core::Entity l_Entity = *(uint64_t *)l_Payload->Data;
          Core::Prefab l_Prefab = *(uint64_t *)l_Payload->Data;
          if (l_Entity.is_alive()) {
            l_Entity.get_transform().position(
                l_Entity.get_transform().get_world_position());
            l_Entity.get_transform().rotation(
                l_Entity.get_transform().get_world_rotation());
            l_Entity.get_transform().scale(
                l_Entity.get_transform().get_world_scale());
            l_Entity.get_transform().set_parent(0);
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

      bool l_OpenedEntryPopup = false;

      for (auto it = Core::Entity::ms_LivingInstances.begin();
           it != Core::Entity::ms_LivingInstances.end(); ++it) {
        if (Core::Component::Transform(it->get_transform().get_parent())
                .is_alive()) {
          continue;
        }
        if (render_entity_hierarchy(*it, &l_OpenedEntryPopup, p_Delta)) {
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
                  Helper::create_handle_transaction(l_Entity));

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
