#include "LowEditorMainWindow.h"

#include "imgui.h"

#include "IconsFontAwesome5.h"

#include "LowEditorLogWidget.h"
#include "LowEditorRenderFlowWidget.h"
#include "LowEditorDetailsWidget.h"
#include "LowEditorProfilerWidget.h"
#include "LowEditorEditingWidget.h"
#include "LowEditorAssetWidget.h"

#include "LowUtilContainers.h"

#include "LowRendererTexture2D.h"

#include <functional>

namespace Low {
  namespace Editor {
    const int g_DockSpaceId = 4785;
    bool g_CentralDockOpen = true;

    LogWidget g_LogWidget;
    EditingWidget *g_MainViewportWidget;
    DetailsWidget *g_DetailsWidget;
    ProfilerWidget *g_ProfilerWidget;
    AssetWidget *g_AssetWidget;

    Core::Entity g_SelectedEntity;

    void set_selected_entity(Core::Entity p_Entity)
    {
      g_SelectedEntity = p_Entity;

      g_DetailsWidget->clear();

      if (!g_SelectedEntity.is_alive()) {
        uint32_t l_Id = ~0u;
        g_MainViewportWidget->m_RenderFlowWidget->get_renderflow()
            .get_resources()
            .get_buffer_resource(N(SelectedIdBuffer))
            .set(&l_Id);
        return;
      }
      uint32_t l_Id = p_Entity.get_index();

      g_MainViewportWidget->m_RenderFlowWidget->get_renderflow()
          .get_resources()
          .get_buffer_resource(N(SelectedIdBuffer))
          .set(&l_Id);

      for (auto it = g_SelectedEntity.get_components().begin();
           it != g_SelectedEntity.get_components().end(); ++it) {
        g_DetailsWidget->add_section(it->second);
      }
    }

    Core::Entity get_selected_entity()
    {
      return g_SelectedEntity;
    }

    static void render_menu_bar(float p_Delta)
    {
      // Menu
      if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Scene")) {
          if (ImGui::MenuItem("Save", NULL, nullptr)) {
          }

          ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
      }
    }

    static void render_central_docking_space(float p_Delta)
    {
      // We are using the ImGuiWindowFlags_NoDocking flag to make the parent
      // window not dockable into, because it would be confusing to have two
      // docking targets within each others.
      ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
      const ImGuiViewport *viewport = ImGui::GetMainViewport();
      static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

      ImGui::SetNextWindowPos(viewport->WorkPos);
      ImGui::SetNextWindowSize(viewport->WorkSize);
      ImGui::SetNextWindowViewport(viewport->ID);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
      window_flags |= ImGuiWindowFlags_NoTitleBar |
                      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                      ImGuiWindowFlags_NoMove;
      window_flags |=
          ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

      if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
      ImGui::Begin("DockSpace", &g_CentralDockOpen, window_flags);
      ImGui::PopStyleVar(3);

      ImGuiIO &io = ImGui::GetIO();
      ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
      ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

      ImGui::End();
    }

    void initialize()
    {
      LogWidget::initialize();

      g_MainViewportWidget = new EditingWidget();

      g_DetailsWidget = new DetailsWidget();
      g_ProfilerWidget = new ProfilerWidget();
      g_AssetWidget = new AssetWidget();
    }

    void tick(float p_Delta)
    {
      render_menu_bar(p_Delta);

      render_central_docking_space(p_Delta);

      g_LogWidget.render(p_Delta);
      g_MainViewportWidget->render(p_Delta);
      g_DetailsWidget->render(p_Delta);
      g_ProfilerWidget->render(p_Delta);
      g_AssetWidget->render(p_Delta);

      // ImGui::ShowDemoWindow();
    }
  } // namespace Editor
} // namespace Low
