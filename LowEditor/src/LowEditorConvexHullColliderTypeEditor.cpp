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
#include "LowMathVectorUtil.h"
#include "LowRendererMesh.h"
#include "LowRendererMeshResource.h"
#include "LowUtilContainers.h"
#include "LowUtilHandle.h"
#include "LowUtilLogger.h"
#include "LowUtilString.h"
#include "LowUtilJobManager.h"
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

      Renderer::Mesh l_Mesh = Util::Handle::DEAD;
      if (l_MeshRenderer.is_alive()) {
        l_Mesh = l_MeshRenderer.get_mesh();
      }

      if (ImGui::BeginPopupModal("Construct collider from mesh")) {

        if (!l_MeshRenderer.is_alive()) {
          ImGui::Text("This entity does not have a MeshRenderer "
                      "component assigned.");

          if (Gui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
          }
        } else if (!l_Mesh.is_alive()) {
          ImGui::Text("This entity's mesh renderer does not have a "
                      "valid mesh "
                      "assigned.");

          if (Gui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
          }
        } else {
          ImGui::Text(
              "The existing collider will be wiped and replaced with "
              "a version constructed from the actual mesh data.");

          if (Gui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
          }
          ImGui::SameLine();
          if (Gui::Button("Load")) {

            Renderer::MeshResource l_MeshResource =
                l_Mesh.get_resource();

            const u64 l_ColliderId = l_Collider.get_id();

            Util::JobManager::IO::schedule_read_mesh(
                l_MeshResource.get_mesh_path(),
                l_MeshResource.get_sidecar_path(),
                [l_ColliderId](
                    bool p_Success,
                    Util::JobManager::IO::MeshLoadResult p_Result) {
                  if (!p_Success) {
                    LOW_LOG_ERROR << "Failed to load mesh file."
                                  << LOW_LOG_END;
                    return;
                  }

                  Core::Component::ConvexHullCollider l_Collider =
                      l_ColliderId;

                  l_Collider.get_points().clear();

                  for (u32 i = 0; i < p_Result.mesh.submeshes.size();
                       ++i) {
                    const Util::Resource::Submesh &i_Submesh =
                        p_Result.mesh.submeshes[i];
                    for (u32 j = 0; j < i_Submesh.meshInfos.size();
                         ++j) {
                      const Util::Resource::MeshInfo i_MeshInfo =
                          i_Submesh.meshInfos[j];

                      for (u32 k = 0; k < i_MeshInfo.vertices.size();
                           ++k) {
                        const Math::Vector3 i_Point = Math::Vector3(
                            i_Submesh.transform *
                            Math::Vector4(
                                i_MeshInfo.vertices[k].position,
                                1.0f));

                        bool i_Unique = true;

                        for (const Math::Vector3 &i_ExPoint :
                             l_Collider.get_points()) {
                          const float i_Distance =
                              Math::VectorUtil::distance_squared(
                                  i_Point, i_ExPoint);
                          if (i_Distance < 0.5f) {
                            i_Unique = false;
                            break;
                          }
                        }

                        if (i_Unique) {
                          l_Collider.get_points().push_back(i_Point);
                        }
                      }
                    }
                  }
                  l_Collider.mark_dirty();
                  LOW_LOG_INFO << "Added "
                               << l_Collider.get_points().size()
                               << " from mesh." << LOW_LOG_END;
                });

            Util::JobManager::Background::schedule(
                "Loading convex collider from mesh",
                [](Util::Function<void(float)> p_Progress) {
                  p_Progress(0);
                });
            ImGui::CloseCurrentPopup();
          }
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
