#include "LowEditorMainWindow.h"

#include "imgui.h"

#include "LowEditorLogWidget.h"
#include "LowEditorRenderFlowWidget.h"
#include "LowEditorDetailsWidget.h"
#include "LowEditorProfilerWidget.h"

#include "LowUtilContainers.h"

#include "LowRendererTexture2D.h"

#include <functional>

namespace Low {
  namespace Editor {
    const int g_DockSpaceId = 4785;
    bool g_CentralDockOpen = true;

    LogWidget g_LogWidget;
    RenderFlowWidget *g_MainViewportWidget;
    DetailsWidget *g_DetailsWidget;
    ProfilerWidget *g_ProfilerWidget;

    static void render_menu_bar(float p_Delta)
    {
      // Menu
      if (ImGui::BeginMainMenuBar()) {
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
      g_MainViewportWidget = new RenderFlowWidget(
          "Viewport", Renderer::RenderFlow::ms_LivingInstances[0]);

      g_DetailsWidget = new DetailsWidget();
      g_ProfilerWidget = new ProfilerWidget();
    }

    void tick(float p_Delta)
    {
      render_menu_bar(p_Delta);

      render_central_docking_space(p_Delta);

      if (ImGui::Button("RenderFlow")) {
        g_DetailsWidget->add_section(
            Renderer::RenderFlow::living_instances()[0]);
      }
      if (ImGui::Button("Context")) {
        g_DetailsWidget->add_section(
            Renderer::Interface::Context::living_instances()[0]);
      }
      if (ImGui::Button("Texture2D")) {
        g_DetailsWidget->add_section(
            Renderer::Texture2D::living_instances()[1]);
      }

      if (ImGui::Button("Clear")) {
        g_DetailsWidget->clear();
      }

      g_LogWidget.render(p_Delta);
      g_MainViewportWidget->render(p_Delta);
      g_DetailsWidget->render(p_Delta);
      g_ProfilerWidget->render(p_Delta);
    }
  } // namespace Editor
} // namespace Low
