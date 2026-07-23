#include "LowEditorNavigationDebugWidget.h"

#include "LowCoreDebugGeometry.h"
#include "LowCoreNavigation.h"
#include "LowCoreScene.h"
#include "LowEditorGui.h"
#include "LowEditorIcons.h"
#include "LowEditorPropertyEditors.h"
#include "LowUtilLogger.h"

#include "imgui.h"

namespace Low {
  namespace Editor {
    namespace {
      bool render_setting_value(const char *p_Label, float *p_Value,
                                float p_Speed, float p_Min,
                                float p_Max)
      {
        return PropertyEditors::render_line(p_Label, [&]() {
          return Gui::DragFloatWithButtons("##value", p_Value,
                                           p_Speed, p_Min, p_Max);
        });
      }

      bool render_setting_value(const char *p_Label, int *p_Value,
                                int p_Speed, int p_Min, int p_Max)
      {
        return PropertyEditors::render_line(p_Label, [&]() {
          return Gui::DragIntWithButtons("##value", p_Value,
                                         p_Speed, p_Min, p_Max);
        });
      }

      bool render_vector_value(const char *p_Label,
                               Math::Vector3 &p_Value)
      {
        return PropertyEditors::render_line(p_Label, [&]() {
          return Gui::Vector3Edit(p_Value);
        });
      }

      void apply_settings_to_loaded_scene(
          const Core::Navigation::BuildSettings &p_Settings)
      {
        Core::Scene l_Scene = Core::Scene::get_loaded_scene();
        if (!l_Scene.is_alive()) {
          return;
        }

        Core::Navigation::World l_NavigationWorld =
            l_Scene.get_navigation_world();
        if (!l_NavigationWorld.is_alive()) {
          return;
        }

        Core::Navigation::apply_build_settings(l_NavigationWorld,
                                               p_Settings);
      }

      bool render_build_settings(
          Core::Navigation::BuildSettings &p_Settings)
      {
        if (!Gui::CollapsibleHeader("Build settings",
                                    LOW_EDITOR_ICON_ROUTE)) {
          return false;
        }

        bool l_Changed = false;
        l_Changed |= render_setting_value(
            "Agent radius", &p_Settings.agent_radius, 0.01f, 0.0f,
            10.0f);
        l_Changed |= render_setting_value(
            "Agent height", &p_Settings.agent_height, 0.01f, 0.0f,
            20.0f);
        l_Changed |= render_setting_value(
            "Max slope", &p_Settings.agent_max_slope, 0.1f, 0.0f,
            90.0f);
        l_Changed |= render_setting_value(
            "Max climb", &p_Settings.agent_max_climb, 0.01f, 0.0f,
            10.0f);
        l_Changed |= render_setting_value(
            "Cell size", &p_Settings.cell_size, 0.01f, 0.01f,
            10.0f);
        l_Changed |= render_setting_value(
            "Cell height", &p_Settings.cell_height, 0.01f, 0.01f,
            10.0f);
        l_Changed |= render_setting_value("Tile size",
                                          &p_Settings.tile_size, 1,
                                          1, 4096);

        return l_Changed;
      }

      bool render_path_query(Math::Vector3 &p_Start,
                             Math::Vector3 &p_End,
                             Math::Vector3 &p_HalfExtents)
      {
        if (!Gui::CollapsibleHeader("Path query",
                                    LOW_EDITOR_ICON_ROUTE)) {
          return false;
        }

        render_vector_value("Start", p_Start);
        render_vector_value("End", p_End);
        render_vector_value("Half extents", p_HalfExtents);

        return Gui::Button("Find path", false,
                           LOW_EDITOR_ICON_ROUTE);
      }

      const char *get_tile_state_label(
          Core::Navigation::TileState p_State)
      {
        switch (p_State) {
        case Core::Navigation::TileState::Empty:
          return "Empty";
        case Core::Navigation::TileState::Dirty:
          return "Dirty";
        case Core::Navigation::TileState::Queued:
          return "Queued";
        case Core::Navigation::TileState::Building:
          return "Building";
        case Core::Navigation::TileState::Ready:
          return "Ready";
        case Core::Navigation::TileState::Failed:
          return "Failed";
        }

        return "Unknown";
      }

      void render_runtime_stats()
      {
        if (!Gui::CollapsibleHeader("Runtime",
                                    LOW_EDITOR_ICON_ROUTE)) {
          return;
        }

        Core::Scene l_Scene = Core::Scene::get_loaded_scene();
        if (!l_Scene.is_alive()) {
          ImGui::TextDisabled("No loaded scene.");
          return;
        }

        Core::Navigation::World l_NavigationWorld =
            l_Scene.get_navigation_world();
        if (!l_NavigationWorld.is_alive()) {
          ImGui::TextDisabled("Loaded scene has no navigation world.");
          return;
        }

        Low::Util::List<Core::Navigation::Tile> l_Tiles;
        Core::Navigation::collect_tiles(l_NavigationWorld, &l_Tiles);

        u32 l_EmptyCount = 0u;
        u32 l_DirtyCount = 0u;
        u32 l_QueuedCount = 0u;
        u32 l_BuildingCount = 0u;
        u32 l_ReadyCount = 0u;
        u32 l_FailedCount = 0u;

        for (const Core::Navigation::Tile &i_Tile : l_Tiles) {
          switch (i_Tile.state) {
          case Core::Navigation::TileState::Empty:
            ++l_EmptyCount;
            break;
          case Core::Navigation::TileState::Dirty:
            ++l_DirtyCount;
            break;
          case Core::Navigation::TileState::Queued:
            ++l_QueuedCount;
            break;
          case Core::Navigation::TileState::Building:
            ++l_BuildingCount;
            break;
          case Core::Navigation::TileState::Ready:
            ++l_ReadyCount;
            break;
          case Core::Navigation::TileState::Failed:
            ++l_FailedCount;
            break;
          }
        }

        ImGui::TextDisabled(
            "Revision: %llu",
            static_cast<unsigned long long>(
                l_NavigationWorld.get_navmesh_revision()));
        ImGui::TextDisabled(
            "Tiles: %u  Build queue: %u",
            static_cast<u32>(l_Tiles.size()),
            Core::Navigation::get_queued_tile_count(
                l_NavigationWorld));
        ImGui::TextDisabled("Empty: %u  Dirty: %u  Queued: %u",
                            l_EmptyCount, l_DirtyCount,
                            l_QueuedCount);
        ImGui::TextDisabled("Building: %u  Ready: %u  Failed: %u",
                            l_BuildingCount, l_ReadyCount,
                            l_FailedCount);

        if (Gui::CollapsibleHeader("Tiles",
                                   LOW_EDITOR_ICON_ROUTE)) {
          for (const Core::Navigation::Tile &i_Tile : l_Tiles) {
            ImGui::TextDisabled(
                "(%d, %d)  %s", i_Tile.coord.x, i_Tile.coord.z,
                get_tile_state_label(i_Tile.state));
          }
        }
      }
    } // namespace

    void NavigationDebugWidget::render(float p_Delta)
    {
      (void)p_Delta;

      ImGui::Begin(LOW_EDITOR_ICON_ROUTE " Navigation", &m_Open);

      if (!m_BuildSettingsInitialized) {
        m_BuildSettings = Core::Navigation::get_project_build_settings();
        m_BuildSettingsInitialized = true;
      }

      if (render_build_settings(m_BuildSettings)) {
        apply_settings_to_loaded_scene(m_BuildSettings);
        m_SettingsSaveMessage = "Unsaved settings.";
      }

      if (Gui::SaveButton()) {
        if (Core::Navigation::save_project_build_settings(
                m_BuildSettings)) {
          m_SettingsSaveMessage = "Settings saved.";
        } else {
          m_SettingsSaveMessage = "Could not save settings.";
        }
      }

      if (!m_SettingsSaveMessage.empty()) {
        ImGui::TextDisabled("%s", m_SettingsSaveMessage.c_str());
      }

      if (render_path_query(m_PathStart, m_PathEnd,
                            m_PathHalfExtents)) {
        Core::Scene l_Scene = Core::Scene::get_loaded_scene();
        m_PathQueryAttempted = true;
        m_LastPathQuerySucceeded = false;
        m_LastPathResult.points.clear();
        m_LastPathResult.partial = false;

        if (!l_Scene.is_alive()) {
          m_LastPathQueryMessage = "No loaded scene.";
        } else {
          Core::Navigation::World l_NavigationWorld =
              l_Scene.get_navigation_world();
          if (!l_NavigationWorld.is_alive()) {
            m_LastPathQueryMessage =
                "Loaded scene has no navigation world.";
          } else {
            m_LastPathQuerySucceeded =
                l_NavigationWorld.find_path(
                    m_PathStart, m_PathEnd, m_PathHalfExtents,
                    &m_LastPathResult);

            if (!m_LastPathQuerySucceeded) {
              m_LastPathQueryMessage = "Path query failed.";
            } else if (m_LastPathResult.partial) {
              m_LastPathQueryMessage = "Partial path found.";
            } else {
              m_LastPathQueryMessage = "Path found.";
            }
          }
        }
      }

      if (m_PathQueryAttempted) {
        ImGui::TextColored(
            m_LastPathQuerySucceeded
                ? ImVec4(0.45f, 0.85f, 0.45f, 1.0f)
                : ImVec4(0.95f, 0.45f, 0.35f, 1.0f),
            "%s", m_LastPathQueryMessage.c_str());
        ImGui::TextDisabled(
            "Path points: %u",
            static_cast<u32>(m_LastPathResult.points.size()));
        ImGui::TextDisabled(
            "Path revision: %llu",
            static_cast<unsigned long long>(
                m_LastPathResult.navmesh_revision));
      }

      if (!m_LastPathResult.points.empty()) {
        Core::DebugGeometry::render_path(
            m_LastPathResult.points,
            m_LastPathResult.partial
                ? Math::Color(1.0f, 0.75f, 0.15f, 1.0f)
                : Math::Color(0.25f, 0.9f, 1.0f, 1.0f),
            false, 0.16f, 0.05f);
      }

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
            Core::Navigation::apply_build_settings(l_NavigationWorld,
                                                   m_BuildSettings);
            const bool l_Built =
                l_Collected &&
                l_NavigationWorld.build_from_geometry(&l_Geometry);
            m_LastVertexCount =
                static_cast<u32>(l_Geometry.vertices.size());
            m_LastTriangleCount =
                static_cast<u32>(l_Geometry.indices.size() / 3u);
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

      render_runtime_stats();

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

      bool l_RenderTiles =
          Core::Navigation::is_tile_debug_rendering_enabled();
      if (Gui::Checkbox("Render navigation tiles", &l_RenderTiles)) {
        Core::Navigation::set_tile_debug_rendering_enabled(
            l_RenderTiles);
      }

      bool l_RenderInvokers =
          Core::Navigation::is_invoker_debug_rendering_enabled();
      if (Gui::Checkbox("Render navigation invokers",
                        &l_RenderInvokers)) {
        Core::Navigation::set_invoker_debug_rendering_enabled(
            l_RenderInvokers);
      }

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
