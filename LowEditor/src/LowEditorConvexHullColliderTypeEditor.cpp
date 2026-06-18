#include "LowEditorConvexHullColliderTypeEditor.h"

#include "LowCoreConvexHullCollider.h"
#include "LowEditorGui.h"
#include <imgui.h>

#include "LowEditorMainWindow.h"
#include "LowEditorEditingWidget.h"
#include "LowEditorConvexHullEditingLayer.h"

#include "LowMath.h"
#include "LowUtilContainers.h"
#include "LowUtilString.h"
namespace Low {
  namespace Editor {

    void ConvexHullColliderTypeEditor::render(const float p_Delta)
    {
      default_render(p_Delta);

      ImGui::Dummy(ImVec2(0, 5));

      if (Gui::EditButton("Edit shape")) {
        get_editing_widget()->m_Viewport->get_editing_layers().push(
            Util::make_shared<ConvexHullEditingLayer>(
                m_Handle.get_id()));
      }

      Core::Component::ConvexHullCollider l_Collider =
          m_Handle.get_id();

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
