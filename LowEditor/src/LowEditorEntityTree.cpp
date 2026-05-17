#include "LowEditorEntityTree.h"

#include "imgui.h"

#include "LowEditor.h"
#include "LowEditorCommonOperations.h"
#include "LowEditorGui.h"
#include "LowEditorIcons.h"

#include "LowCoreDirectionalLight.h"
#include "LowCoreMeshRenderer.h"
#include "LowCorePointLight.h"
#include "LowCoreTransform.h"

#include "LowMathQuaternionUtil.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Low {
  namespace Editor {
    namespace EntityTree {
      const char *get_icon(Core::Entity p_Entity)
      {
        if (p_Entity.has_component(
                Core::Component::PointLight::type_id())) {
          return LOW_EDITOR_ICON_POINT_LIGHT;
        }
        if (p_Entity.has_component(
                Core::Component::DirectionalLight::type_id())) {
          return LOW_EDITOR_ICON_DIRECTIONAL_LIGHT;
        }
        if (p_Entity.has_component(
                Core::Component::MeshRenderer::type_id())) {
          return LOW_EDITOR_ICON_CUBE;
        }
        return LOW_EDITOR_ICON_CYLINDER;
      }

      bool matches_search(Core::Entity p_Entity,
                          const Util::String &p_Search)
      {
        if (p_Search.empty()) {
          return true;
        }

        Util::String l_LowName = p_Entity.get_name().c_str();
        l_LowName.make_lower();
        if (Util::StringHelper::contains(l_LowName, p_Search)) {
          return true;
        }

        for (auto it = p_Entity.get_transform().get_children().begin();
             it != p_Entity.get_transform().get_children().end();
             ++it) {
          Core::Component::Transform i_Transform = *it;
          if (!i_Transform.is_alive()) {
            continue;
          }
          if (matches_search(i_Transform.get_entity(), p_Search)) {
            return true;
          }
        }

        return false;
      }

      void detach_from_parent_preserve_world(Core::Entity p_Entity)
      {
        p_Entity.get_transform().position(
            p_Entity.get_transform().get_world_position());
        p_Entity.get_transform().rotation(
            p_Entity.get_transform().get_world_rotation());
        p_Entity.get_transform().scale(
            p_Entity.get_transform().get_world_scale());
        p_Entity.get_transform().set_parent(0);
      }

      static bool entity_is_visible(Core::Entity p_Entity,
                                    const RenderOptions &p_Options)
      {
        return !p_Options.filterToRegion ||
               p_Entity.get_region() == p_Options.regionFilter;
      }

      static bool render_actions(Core::Entity p_Entity,
                                 bool *p_OpenedEntryPopup,
                                 const RenderOptions &p_Options)
      {
        bool l_Break = false;

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
                History::destroy_handle_transaction(p_Entity));
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

              Math::Matrix4x4 l_InverseParentMatrix = glm::inverse(
                  p_Entity.get_transform().get_world_matrix());

              Math::Matrix4x4 l_LocalMatrix =
                  l_InverseParentMatrix *
                  l_Entity.get_transform().get_world_matrix();

              glm::vec3 scale;
              glm::quat rotation;
              glm::vec3 translation;

              glm::vec3 skew;
              glm::vec4 perspective;
              glm::decompose(l_LocalMatrix, scale, rotation,
                             translation, skew, perspective);

              Transaction l_Transaction("Parent entity");
              l_Transaction.add_operation(
                  new CommonOperations::PropertyEditOperation(
                      l_Entity.get_transform(), N(position),
                      l_Entity.get_transform().position(),
                      translation));
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
                      Util::Variant::from_handle(
                          p_Entity.get_transform())));

              get_global_changelist().add_entry(l_Transaction);

              l_Entity.get_transform().position(translation);
              l_Entity.get_transform().rotation(rotation);
              l_Entity.get_transform().scale(scale);

              l_Entity.get_transform().set_parent(
                  p_Entity.get_transform().get_id());

              if (p_Options.moveDroppedEntitiesToParentRegion &&
                  p_Entity.get_region().is_alive()) {
                p_Entity.get_region().add_entity(l_Entity);
              }
            }
          }

          ImGui::EndDragDropTarget();
        }

        return l_Break;
      }

      bool render(Core::Entity p_Entity, bool *p_OpenedEntryPopup,
                  const Util::String &p_Search,
                  const RenderOptions &p_Options)
      {
        bool l_Break = false;

        Core::Component::Transform l_Transform =
            p_Entity.get_transform();
        const bool l_Leaf = l_Transform.get_children().empty();

        ImGuiTreeNodeFlags l_Flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        if (l_Leaf) {
          l_Flags |= ImGuiTreeNodeFlags_Leaf |
                     ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }
        if (is_entity_selected(p_Entity)) {
          l_Flags |= ImGuiTreeNodeFlags_Selected;
        }
        if (!p_Search.empty()) {
          l_Flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        Util::String l_IdString = LOW_TO_STRING(p_Entity.get_id());
        ImGui::PushID(l_IdString.c_str());
        const bool l_Open = ImGui::TreeNodeEx(
            "##entity_row", l_Flags, "%s %s", get_icon(p_Entity),
            p_Entity.get_name().c_str());

        ImGuiIO &io = ImGui::GetIO();
        if (ImGui::IsItemClicked()) {
          if (io.KeyCtrl) {
            add_entity_selection(p_Entity);
          } else {
            set_selected_entity(p_Entity);
          }
        }

        l_Break =
            render_actions(p_Entity, p_OpenedEntryPopup, p_Options);
        if (l_Break) {
          if (!l_Leaf && l_Open) {
            ImGui::TreePop();
          }
          ImGui::PopID();
          return true;
        }

        if (!l_Leaf && l_Open) {
          for (auto it = l_Transform.get_children().begin();
               it != l_Transform.get_children().end(); ++it) {
            Core::Component::Transform i_Transform = *it;
            if (!i_Transform.is_alive()) {
              continue;
            }

            Core::Entity i_Entity = i_Transform.get_entity();
            if (!entity_is_visible(i_Entity, p_Options)) {
              continue;
            }
            if (!matches_search(i_Entity, p_Search)) {
              continue;
            }

            bool i_Break = render(i_Entity, p_OpenedEntryPopup,
                                  p_Search, p_Options);
            if (!l_Break) {
              l_Break = i_Break;
            }
            if (l_Break) {
              break;
            }
          }
          ImGui::TreePop();
        }

        ImGui::PopID();
        return l_Break;
      }
    } // namespace EntityTree
  } // namespace Editor
} // namespace Low
