#include "LowEditorJobWidget.h"

#include "LowEditorGui.h"
#include "LowEditorThemes.h"

#include "LowUtilJobManager.h"

#include <imgui.h>
#include "IconsLucide.h"

#include <algorithm>
#include <cfloat>
#include <cstdio>

namespace Low {
  namespace Editor {
    static const char *
    get_status_label(Util::JobManager::Background::JobStatus p_Status)
    {
      using namespace Util::JobManager::Background;

      switch (p_Status) {
      case JobStatus::Pending:
        return "Pending";
      case JobStatus::Running:
        return "Running";
      case JobStatus::Completed:
        return "Completed";
      case JobStatus::Failed:
        return "Failed";
      }

      return "Unknown";
    }

    static Math::Color
    get_status_color(Util::JobManager::Background::JobStatus p_Status)
    {
      Theme &l_Theme = theme_get_current();
      using namespace Util::JobManager::Background;

      switch (p_Status) {
      case JobStatus::Pending:
        return l_Theme.warning;
      case JobStatus::Running:
        return l_Theme.info;
      case JobStatus::Completed:
        return l_Theme.success;
      case JobStatus::Failed:
        return l_Theme.error;
      }

      return l_Theme.textDisabled;
    }

    static bool
    is_done(const Util::JobManager::Tracking::JobSnapshot &p_Job)
    {
      using namespace Util::JobManager::Background;
      return p_Job.status == JobStatus::Completed ||
             p_Job.status == JobStatus::Failed;
    }

    static void render_metric(const char *p_Label, uint32_t p_Value,
                              const Math::Color &p_Color)
    {
      Theme &l_Theme = theme_get_current();
      ImDrawList *l_DrawList = ImGui::GetWindowDrawList();
      const ImVec2 l_Pos = ImGui::GetCursorScreenPos();
      const float l_Width = ImGui::GetContentRegionAvail().x;
      const float l_Height = 30.0f;
      const ImVec2 l_Max(l_Pos.x + l_Width, l_Pos.y + l_Height);

      l_DrawList->AddRectFilled(
          l_Pos, l_Max, color_to_imcolor(l_Theme.input), 3.0f);
      l_DrawList->AddRectFilled(l_Pos,
                                ImVec2(l_Pos.x + 3.0f, l_Max.y),
                                color_to_imcolor(p_Color), 3.0f,
                                ImDrawFlags_RoundCornersLeft);

      ImGui::SetCursorScreenPos(
          ImVec2(l_Pos.x + 9.0f, l_Pos.y + 5.0f));
      ImGui::TextUnformatted(p_Label);

      char l_Buffer[16];
      snprintf(l_Buffer, sizeof(l_Buffer), "%u", p_Value);
      const ImVec2 l_ValueSize = ImGui::CalcTextSize(l_Buffer);
      ImGui::SetCursorScreenPos(
          ImVec2(l_Max.x - l_ValueSize.x - 9.0f, l_Pos.y + 5.0f));
      ImGui::TextUnformatted(l_Buffer);

      ImGui::SetCursorScreenPos(ImVec2(l_Pos.x, l_Max.y + 5.0f));
    }

    static void render_job_table(
        const char *p_Id,
        const Util::List<Util::JobManager::Tracking::JobSnapshot>
            &p_Jobs,
        Util::JobManager::Tracking::JobType p_Type)
    {
      Theme &l_Theme = theme_get_current();

      ImGui::PushStyleColor(ImGuiCol_TableHeaderBg,
                            color_to_imvec4(l_Theme.header));
      ImGui::PushStyleColor(ImGuiCol_TableRowBg,
                            color_to_imvec4(l_Theme.base));
      ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt,
                            color_to_imvec4(l_Theme.input));

      if (ImGui::BeginTable(p_Id, 5,
                            ImGuiTableFlags_RowBg |
                                ImGuiTableFlags_BordersInnerV |
                                ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn(
            "State", ImGuiTableColumnFlags_WidthFixed, 82.0f);
        ImGui::TableSetupColumn("Job");
        ImGui::TableSetupColumn(
            "Detail", ImGuiTableColumnFlags_WidthStretch, 1.4f);
        ImGui::TableSetupColumn(
            "Progress", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn(
            "Time", ImGuiTableColumnFlags_WidthFixed, 58.0f);
        ImGui::TableHeadersRow();

        uint32_t l_Count = 0;
        for (const auto &i_Job : p_Jobs) {
          if (i_Job.type != p_Type) {
            continue;
          }

          l_Count++;
          ImGui::TableNextRow();

          ImGui::TableSetColumnIndex(0);
          ImGui::PushStyleColor(
              ImGuiCol_Text,
              color_to_imvec4(get_status_color(i_Job.status)));
          ImGui::TextUnformatted(get_status_label(i_Job.status));
          ImGui::PopStyleColor();

          ImGui::TableSetColumnIndex(1);
          ImGui::TextUnformatted(i_Job.label.c_str());

          ImGui::TableSetColumnIndex(2);
          if (i_Job.detail.empty()) {
            ImGui::TextDisabled("-");
          } else {
            ImGui::TextUnformatted(i_Job.detail.c_str());
          }

          ImGui::TableSetColumnIndex(3);
          ImGui::PushStyleColor(
              ImGuiCol_PlotHistogram,
              color_to_imvec4(get_status_color(i_Job.status)));
          ImGui::ProgressBar(i_Job.progress, ImVec2(-FLT_MIN, 0.0f),
                             "");
          ImGui::PopStyleColor();

          ImGui::TableSetColumnIndex(4);
          if (i_Job.elapsedMs >= 1000) {
            ImGui::Text("%.1fs", static_cast<float>(i_Job.elapsedMs) /
                                     1000.0f);
          } else {
            ImGui::Text("%llums", static_cast<unsigned long long>(
                                      i_Job.elapsedMs));
          }
        }

        if (l_Count == 0) {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::TextDisabled("Idle");
          ImGui::TableSetColumnIndex(1);
          ImGui::TextDisabled("No jobs");
        }

        ImGui::EndTable();
      }

      ImGui::PopStyleColor(3);
    }

    void JobWidget::render(float p_Delta)
    {

      ImGui::Begin(ICON_LC_LIST_CHECKS " Jobs", &m_Open);

      if (!Util::JobManager::Tracking::is_enabled()) {
        ImGui::TextDisabled(
            "Job tracking is disabled for this build.");
        ImGui::End();
        return;
      }

      Util::List<Util::JobManager::Tracking::JobSnapshot> l_Jobs =
          Util::JobManager::Tracking::get_snapshot();

      std::sort(
          l_Jobs.begin(), l_Jobs.end(),
          [](const Util::JobManager::Tracking::JobSnapshot &p_Left,
             const Util::JobManager::Tracking::JobSnapshot &p_Right) {
            if (is_done(p_Left) != is_done(p_Right)) {
              return !is_done(p_Left);
            }
            return p_Left.id > p_Right.id;
          });

      uint32_t l_ActiveIo = 0;
      uint32_t l_ActiveBackground = 0;
      uint32_t l_Completed = 0;

      for (const auto &i_Job : l_Jobs) {
        if (is_done(i_Job)) {
          l_Completed++;
        } else if (i_Job.type ==
                   Util::JobManager::Tracking::JobType::IO) {
          l_ActiveIo++;
        } else {
          l_ActiveBackground++;
        }
      }

      if (Gui::ClearButton(l_Completed == 0)) {
        Util::JobManager::Tracking::clear_completed();
      }

      ImGui::Spacing();
      render_metric(ICON_LC_HARD_DRIVE " Active IO", l_ActiveIo,
                    theme_get_current().info);
      render_metric(ICON_LC_CPU " Active background",
                    l_ActiveBackground, theme_get_current().profile);
      render_metric(ICON_LC_CHECK_CHECK " Completed", l_Completed,
                    theme_get_current().success);

      ImGui::Spacing();
      if (Gui::CollapsibleHeader("IO Jobs", ICON_LC_HARD_DRIVE,
                                 theme_get_current().info)) {
        render_job_table("IoJobs", l_Jobs,
                         Util::JobManager::Tracking::JobType::IO);
      }

      ImGui::Spacing();
      if (Gui::CollapsibleHeader("Background Jobs", ICON_LC_CPU,
                                 theme_get_current().profile)) {
        render_job_table(
            "BackgroundJobs", l_Jobs,
            Util::JobManager::Tracking::JobType::Background);
      }

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
