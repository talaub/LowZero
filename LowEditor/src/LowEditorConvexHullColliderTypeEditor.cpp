#include "LowEditorConvexHullColliderTypeEditor.h"

#include "LowCoreConvexHullCollider.h"
#include "LowCoreMeshRenderer.h"
#include "LowEditorGui.h"
#include "LowEditorIcons.h"
#include <imgui.h>

#include "LowEditorMainWindow.h"
#include "LowEditorEditingWidget.h"
#include "LowEditorConvexHullEditingLayer.h"

#include "LowEditorThemes.h"
#include "LowMath.h"
#include "LowUtilContainers.h"
#include "LowUtilString.h"
namespace Low {
  namespace Editor {

    void ConvexHullColliderTypeEditor::render(const float p_Delta)
    {
      Core::Component::ConvexHullCollider l_Collider =
          m_Handle.get_id();

      Core::Entity l_Entity = l_Collider.get_entity();
      Core::Component::MeshRenderer l_MeshRenderer =
          l_Entity.get_component(
              Core::Component::MeshRenderer::type_id());

      if (ImGui::BeginPopupModal("Construct collider from mesh")) {

        if (l_MeshRenderer.is_alive()) {
          ImGui::Text(
              "The existing collider will be wiped and replaced with "
              "a version constructed from the actual mesh data.");
        } else {

          ImGui::Text("This entity does not have a MeshRenderer "
                      "component assigned.");
        }

        ImGui::EndPopup();
      }

      default_render(p_Delta);

      ImGui::Dummy(ImVec2(0, 5));

      if (Gui::EditButton("Edit shape")) {
        get_editing_widget()->m_Viewport->get_editing_layers().push(
            Util::make_shared<ConvexHullEditingLayer>(
                m_Handle.get_id()));
      }

      ImGui::SameLine();

      if (Gui::Button("Construct from mesh", false,
                      LOW_EDITOR_ICON_FILE)) {
        ImGui::OpenPopup("Construct collider from mesh");
      }

      /*
      if (!l_Collider.get_points().empty()) {
        ImGui::Separator();
        Gui::Heading2("Points");

        int i = 0;
        for (auto it = l_Collider.get_points().begin();
             it != l_Collider.get_points().end(); i++) {

          ImGui::PushID(i);

          Math::Vector3 &i_Point = *it;

          Util::StringBuilder i_Builder;
          i_Builder.append("Point ").append(i + 1);

          show_line(i_Builder.get(),
                    [&it, l_Collider, &i_Point]() -> bool {
                      if (Gui::Vector3Edit(i_Point)) {
                      }

                      if (Gui::DeleteButton()) {
                        it = l_Collider.get_points().erase(it);
                      } else {
                        ++it;
                      }
                      return false;
                    });

          ImGui::PopID();
        }
      }
      */
    }
  } // namespace Editor
} // namespace Low
