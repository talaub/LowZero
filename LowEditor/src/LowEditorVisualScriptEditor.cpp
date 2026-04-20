#include "LowEditorVisualScriptEditor.h"

#include "LowEditorGui.h"
#include "IconsLucide.h"
#include "LowUtilLogger.h"

#include <imgui.h>

namespace Low {
  namespace Editor {
    namespace VisualScript {
      namespace {
        static void render_toolbar(Editor &p_Editor)
        {
          (void)p_Editor;

          ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                              ImVec2(8.0f, 6.0f));
          ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                              ImVec2(6.0f, 0.0f));
          ImGui::BeginChild(
              "##visual_script_toolbar", ImVec2(0.0f, 38.0f), false,
              ImGuiWindowFlags_NoScrollbar |
                  ImGuiWindowFlags_NoScrollWithMouse |
                  ImGuiWindowFlags_NoSavedSettings);

          if (Gui::SaveButton()) {
            LOW_LOG_DEBUG << "VisualScript save requested"
                          << LOW_LOG_END;
          }

          ImGui::SameLine();
          if (Gui::Button("Compile", false, ICON_LC_PLAY)) {
            LOW_LOG_DEBUG << "VisualScript compile requested"
                          << LOW_LOG_END;
          }

          ImGui::EndChild();
          ImGui::PopStyleVar(2);
        }
      } // namespace

      void Editor::render(const float p_Delta)
      {
        (void)p_Delta;
        render("Graph", Math::Vector2(0.0f, 0.0f));
      }

      void Editor::render(const char *p_Label,
                          const Math::Vector2 &p_Size)
      {
        if (!document) {
          return;
        }

        render_toolbar(*this);

        const ImVec2 l_ContentAvail = ImGui::GetContentRegionAvail();
        const float l_SplitterWidth = 6.0f;
        const float l_MaxSidebarWidth = LOW_MATH_MIN(
            max_sidebar_width,
            LOW_MATH_MAX(min_sidebar_width,
                         l_ContentAvail.x - 220.0f));
        sidebar_width = LOW_MATH_CLAMP(sidebar_width,
                                       min_sidebar_width,
                                       l_MaxSidebarWidth);

        ImGui::BeginChild("##visual_script_sidebar",
                          ImVec2(sidebar_width, 0.0f), true,
                          ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted("Graph");
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        ImGui::TextDisabled("Variables and other graph data will live here.");
        ImGui::EndChild();

        ImGui::SameLine(0.0f, 0.0f);

        ImGui::PushStyleColor(ImGuiCol_Button,
                              IM_COL32(55, 58, 68, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              IM_COL32(78, 82, 95, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              IM_COL32(92, 97, 112, 255));
        ImGui::Button("##visual_script_splitter",
                      ImVec2(l_SplitterWidth, l_ContentAvail.y));
        ImGui::PopStyleColor(3);

        if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
          ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }

        if (ImGui::IsItemActive()) {
          sidebar_width = LOW_MATH_CLAMP(
              sidebar_width + ImGui::GetIO().MouseDelta.x,
              min_sidebar_width, l_MaxSidebarWidth);
        }

        ImGui::SameLine(0.0f, 0.0f);

        ImGui::BeginChild("##visual_script_canvas_host",
                          ImVec2(0.0f, 0.0f), false,
                          ImGuiWindowFlags_NoSavedSettings);

        if (document->canvas.begin(p_Label, p_Size)) {
          NodeGraphEditorContext l_GraphContext{
              document->graph.graph,
              document->canvas,
              &document->schema,
              &document->state,
              document->canvas.get_draw_list(),
              document->canvas.get_canvas_origin(),
              document->canvas.get_canvas_min(),
              document->canvas.get_canvas_max()};
          document->renderer.render(l_GraphContext);
          document->canvas.end();
        }

        ImGui::EndChild();
      }
    } // namespace VisualScript
  } // namespace Editor
} // namespace Low
