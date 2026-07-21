#include "LowEditorNavigationDebugWidget.h"

#include "LowCoreNavigation.h"
#include "LowCoreScene.h"
#include "LowEditorGui.h"
#include "LowEditorIcons.h"
#include "LowUtilLogger.h"

#include "imgui.h"

namespace Low {
  namespace Editor {
    void NavigationDebugWidget::render(float p_Delta)
    {
      (void)p_Delta;

      ImGui::Begin(LOW_EDITOR_ICON_ROUTE " Navigation", &m_Open);

      if (Gui::Button("Build navmesh", false,
                      LOW_EDITOR_ICON_ROUTE)) {
        Core::Navigation::BuildGeometry l_Geometry;
        Core::Scene l_Scene = Core::Scene::get_loaded_scene();
        m_BuildAttempted = true;
        m_LastBuildSucceeded = false;
        m_LastVertexCount = 0u;
        m_LastTriangleCount = 0u;

        if (!l_Scene.is_alive()) {
          m_LastBuildMessage = "No loaded scene.";
        } else {
          Core::Navigation::World l_NavigationWorld =
              l_Scene.get_navigation_world();
          if (!l_NavigationWorld.is_alive()) {
            m_LastBuildMessage =
                "Loaded scene has no navigation world.";
          } else {
            const bool l_Collected =
                Core::Navigation::collect_build_geometry(&l_Geometry);
            const bool l_Built =
                l_Collected &&
                l_NavigationWorld.build_from_geometry(&l_Geometry);
            m_LastVertexCount =
                static_cast<uint32_t>(l_Geometry.vertices.size());
            m_LastTriangleCount =
                static_cast<uint32_t>(l_Geometry.indices.size() / 3u);
            m_LastBuildSucceeded = l_Built;
            if (!l_Collected) {
              m_LastBuildMessage =
                  "No valid navigation source geometry found.";
            } else if (!l_Built) {
              m_LastBuildMessage = "Navmesh build failed.";
            } else {
              m_LastBuildMessage = "Navmesh built.";
            }

            LOW_LOG_INFO
                << "Navigation build collected=" << l_Collected
                << " vertices=" << l_Geometry.vertices.size()
                << " triangles=" << (l_Geometry.indices.size() / 3u)
                << " built=" << l_Built << LOW_LOG_END;

            if (l_Built) {
              Core::Navigation::set_navmesh_debug_rendering_enabled(
                  true);
            }
          }
        }
      }

      if (m_BuildAttempted) {
        ImGui::Spacing();
        ImGui::TextColored(m_LastBuildSucceeded
                               ? ImVec4(0.45f, 0.85f, 0.45f, 1.0f)
                               : ImVec4(0.95f, 0.45f, 0.35f, 1.0f),
                           "%s", m_LastBuildMessage.c_str());
        ImGui::TextDisabled("Vertices: %u  Triangles: %u",
                            m_LastVertexCount, m_LastTriangleCount);
      }

      bool l_RenderBuildGeometry = Core::Navigation::
          is_build_geometry_debug_rendering_enabled();
      if (Gui::Checkbox("Render build geometry",
                        &l_RenderBuildGeometry)) {
        Core::Navigation::set_build_geometry_debug_rendering_enabled(
            l_RenderBuildGeometry);
      }

      bool l_RenderNavmesh =
          Core::Navigation::is_navmesh_debug_rendering_enabled();
      if (Gui::Checkbox("Render navmesh", &l_RenderNavmesh)) {
        Core::Navigation::set_navmesh_debug_rendering_enabled(
            l_RenderNavmesh);
      }

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
