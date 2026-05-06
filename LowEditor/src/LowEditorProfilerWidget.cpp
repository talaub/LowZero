#include "LowEditorProfilerWidget.h"

#include "LowEditorGui.h"
#include "LowEditorThemes.h"

#include "imgui.h"
#include "IconsLucide.h"
#include <algorithm>
#include <cstdio>
#include <string>

#include "LowCoreGameLoop.h"

#include "LowUtilGlobals.h"
#include "LowUtilProfiler.h"

#define BUFFER_SIZE 256

namespace Low {
  namespace Editor {
    float g_FpsBuffer[BUFFER_SIZE];
    float g_FrameMsBuffer[BUFFER_SIZE];

    struct ProfilerScopeStats
    {
      Util::String group;
      Util::String name;
      float totalMs;
      float maxMs;
      uint32_t calls;
    };

    static ImU32 profiler_color(const Util::String &p_Text)
    {
      uint32_t l_Hash = 2166136261u;
      for (auto i_Char : p_Text) {
        l_Hash ^= static_cast<uint32_t>(i_Char);
        l_Hash *= 16777619u;
      }

      const float l_Hue =
          static_cast<float>(l_Hash % 360u) / 360.0f;
      const float l_Value =
          0.70f + static_cast<float>((l_Hash >> 8u) % 20u) / 100.0f;
      return ImGui::ColorConvertFloat4ToU32(
          ImColor::HSV(l_Hue, 0.48f, l_Value));
    }

    static void push_plot_value(float *p_Buffer, float p_Value)
    {
      for (uint32_t i = 0; i < BUFFER_SIZE - 1; ++i) {
        p_Buffer[i] = p_Buffer[i + 1];
      }

      p_Buffer[BUFFER_SIZE - 1] = p_Value;
    }

    static void build_scope_stats(
        const Util::Profiler::Frame &p_Frame,
        Util::List<ProfilerScopeStats> &p_Stats)
    {
      p_Stats.clear();

      for (auto &i_Sample : p_Frame.samples) {
        ProfilerScopeStats *i_Stats = nullptr;

        for (auto &i_Candidate : p_Stats) {
          if (i_Candidate.group == i_Sample.group &&
              i_Candidate.name == i_Sample.name) {
            i_Stats = &i_Candidate;
            break;
          }
        }

        if (!i_Stats) {
          ProfilerScopeStats l_Stats;
          l_Stats.group = i_Sample.group;
          l_Stats.name = i_Sample.name;
          l_Stats.totalMs = 0.0f;
          l_Stats.maxMs = 0.0f;
          l_Stats.calls = 0;
          p_Stats.push_back(l_Stats);
          i_Stats = &p_Stats.back();
        }

        i_Stats->totalMs += i_Sample.durationMs;
        i_Stats->maxMs =
            std::max(i_Stats->maxMs, i_Sample.durationMs);
        i_Stats->calls++;
      }

      std::sort(p_Stats.begin(), p_Stats.end(),
                [](const ProfilerScopeStats &p_Left,
                   const ProfilerScopeStats &p_Right) {
                  return p_Left.totalMs > p_Right.totalMs;
                });
    }

    static void render_section_header(const char *p_Icon,
                                      const char *p_Label)
    {
      Theme &l_Theme = theme_get_current();
      ImGui::PushStyleColor(ImGuiCol_Text,
                            color_to_imvec4(l_Theme.subtext));
      ImGui::Text("%s %s", p_Icon, p_Label);
      ImGui::PopStyleColor();
    }

    static void render_metric(const char *p_Label, const char *p_Value,
                              const Math::Color &p_Color)
    {
      Theme &l_Theme = theme_get_current();
      ImDrawList *l_DrawList = ImGui::GetWindowDrawList();
      const ImVec2 l_Pos = ImGui::GetCursorScreenPos();
      const float l_Width = ImGui::GetContentRegionAvail().x;
      const float l_Height = 32.0f;
      const ImVec2 l_Max(l_Pos.x + l_Width, l_Pos.y + l_Height);

      l_DrawList->AddRectFilled(l_Pos, l_Max,
                                color_to_imcolor(l_Theme.input),
                                3.0f);
      l_DrawList->AddRectFilled(l_Pos,
                                ImVec2(l_Pos.x + 3.0f, l_Max.y),
                                color_to_imcolor(p_Color), 3.0f,
                                ImDrawFlags_RoundCornersLeft);

      ImGui::SetCursorScreenPos(ImVec2(l_Pos.x + 9.0f,
                                       l_Pos.y + 5.0f));
      ImGui::PushStyleColor(ImGuiCol_Text,
                            color_to_imvec4(l_Theme.subtext));
      ImGui::TextUnformatted(p_Label);
      ImGui::PopStyleColor();

      ImGui::SameLine();
      const ImVec2 l_ValueSize = ImGui::CalcTextSize(p_Value);
      ImGui::SetCursorScreenPos(ImVec2(
          l_Max.x - l_ValueSize.x - 9.0f, l_Pos.y + 5.0f));
      ImGui::TextUnformatted(p_Value);

      ImGui::SetCursorScreenPos(ImVec2(l_Pos.x, l_Max.y + 5.0f));
    }

    static void render_plot(const char *p_Id, const float *p_Buffer,
                            float p_Max)
    {
      Theme &l_Theme = theme_get_current();
      ImGui::PushStyleColor(ImGuiCol_FrameBg,
                            color_to_imvec4(l_Theme.input));
      ImGui::PushStyleColor(ImGuiCol_PlotLines,
                            color_to_imvec4(l_Theme.profile));
      ImGui::PlotLines(p_Id, p_Buffer, BUFFER_SIZE, 0, NULL, 0.0f,
                       p_Max, ImVec2(0.0f, 42.0f));
      ImGui::PopStyleColor(2);
    }

    static void render_timeline(
        const Util::Profiler::Frame &p_Frame)
    {
      Theme &l_Theme = theme_get_current();
      const ImVec2 l_CanvasStart = ImGui::GetCursorScreenPos();
      const float l_Width = ImGui::GetContentRegionAvail().x;
      const float l_RowHeight = 16.0f;
      const float l_Height = 120.0f;
      ImDrawList *l_DrawList = ImGui::GetWindowDrawList();

      l_DrawList->AddRectFilled(
          l_CanvasStart,
          ImVec2(l_CanvasStart.x + l_Width,
                 l_CanvasStart.y + l_Height),
          color_to_imcolor(l_Theme.input), 3.0f);
      l_DrawList->AddRect(
          l_CanvasStart,
          ImVec2(l_CanvasStart.x + l_Width,
                 l_CanvasStart.y + l_Height),
          color_to_imcolor(l_Theme.border), 3.0f);

      const float l_FrameDuration =
          std::max(p_Frame.durationMs, 0.001f);

      for (auto &i_Sample : p_Frame.samples) {
        const float l_X =
            l_CanvasStart.x +
            (i_Sample.startMs / l_FrameDuration) * l_Width;
        const float l_EndX =
            l_CanvasStart.x +
            ((i_Sample.startMs + i_Sample.durationMs) /
             l_FrameDuration) *
                l_Width;
        const float l_Y =
            l_CanvasStart.y + 6.0f +
            static_cast<float>(i_Sample.depth) * l_RowHeight;

        if (l_Y + l_RowHeight > l_CanvasStart.y + l_Height) {
          continue;
        }

        const Util::String l_Label =
            i_Sample.group + "::" + i_Sample.name;
        l_DrawList->AddRectFilled(
            ImVec2(l_X, l_Y),
            ImVec2(std::max(l_X + 1.0f, l_EndX),
                   l_Y + l_RowHeight - 3.0f),
            profiler_color(l_Label), 2.0f);

        if (l_EndX - l_X > 90.0f) {
          l_DrawList->AddText(
              ImVec2(l_X + 4.0f, l_Y + 1.0f),
              ImGui::GetColorU32(ImGuiCol_Text), l_Label.c_str());
        }
      }

      ImGui::Dummy(ImVec2(l_Width, l_Height + 4.0f));
    }

    void ProfilerWidget::render(float p_Delta)
    {
      ImGui::Begin(ICON_LC_TIMER " Profiler");

      Theme &l_Theme = theme_get_current();

      bool l_Enabled = Util::Profiler::is_enabled();
      if (Gui::Checkbox("Capture", &l_Enabled, 0.9f)) {
        Util::Profiler::set_enabled(l_Enabled);
      }
      ImGui::SameLine();
      if (Gui::ClearButton()) {
        Util::Profiler::clear();
      }

      Util::List<Util::Profiler::Frame> l_Frames =
          Util::Profiler::get_frames();
      Util::Profiler::Frame l_Frame =
          Util::Profiler::get_latest_frame();

      push_plot_value(g_FpsBuffer, Core::GameLoop::get_fps());
      push_plot_value(g_FrameMsBuffer, l_Frame.durationMs);

      ImGui::Spacing();

      char l_Buffer[64];
      snprintf(l_Buffer, sizeof(l_Buffer), "%.0f",
               static_cast<float>(Core::GameLoop::get_fps()));
      render_metric(ICON_LC_GAUGE " FPS", l_Buffer,
                    l_Theme.success);
      render_plot("##fps", g_FpsBuffer, 160.0f);

      snprintf(l_Buffer, sizeof(l_Buffer), "%.2f ms  %u scopes",
               l_Frame.durationMs,
               static_cast<uint32_t>(l_Frame.samples.size()));
      render_metric(ICON_LC_ACTIVITY " Frame", l_Buffer,
                    l_Theme.profile);
      render_plot("##frame-ms", g_FrameMsBuffer, 40.0f);

      if (Util::Globals::has(N(LOW_RENDERER_DRAWCALLS))) {
        int l_DrawCalls =
            (int)Util::Globals::get(N(LOW_RENDERER_DRAWCALLS));

        snprintf(l_Buffer, sizeof(l_Buffer), "%i", l_DrawCalls);
        render_metric(ICON_LC_BOXES " Drawcalls", l_Buffer,
                      l_Theme.info);
      } else {
        render_metric(ICON_LC_BOXES " Drawcalls", "n/a",
                      l_Theme.textDisabled);
      }

      if (Util::Globals::has(N(LOW_RENDERER_COMPUTEDISPATCH))) {
        int l_ComputeDispatches =
            (int)Util::Globals::get(N(LOW_RENDERER_COMPUTEDISPATCH));

        snprintf(l_Buffer, sizeof(l_Buffer), "%i",
                 l_ComputeDispatches);
        render_metric(ICON_LC_CPU " Compute", l_Buffer,
                      l_Theme.debug);
      } else {
        render_metric(ICON_LC_CPU " Compute", "n/a",
                      l_Theme.textDisabled);
      }

      Util::List<ProfilerScopeStats> l_Stats;
      build_scope_stats(l_Frame, l_Stats);

      ImGui::Spacing();
      render_section_header(ICON_LC_LIST_TREE, "Scopes");
      ImGui::PushStyleColor(ImGuiCol_TableHeaderBg,
                            color_to_imvec4(l_Theme.header));
      ImGui::PushStyleColor(ImGuiCol_TableRowBg,
                            color_to_imvec4(l_Theme.base));
      ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt,
                            color_to_imvec4(l_Theme.input));
      ImGui::BeginChild("ProfilerScopeList", ImVec2(0.0f, 165.0f),
                        true);
      if (ImGui::BeginTable("ProfilerScopes", 5,
                            ImGuiTableFlags_RowBg |
                                ImGuiTableFlags_BordersInnerV |
                                ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn(
            "Group", ImGuiTableColumnFlags_WidthFixed, 72.0f);
        ImGui::TableSetupColumn("Scope");
        ImGui::TableSetupColumn(
            "Total ms", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn(
            "Max ms", ImGuiTableColumnFlags_WidthFixed, 62.0f);
        ImGui::TableSetupColumn(
            "Calls", ImGuiTableColumnFlags_WidthFixed, 42.0f);
        ImGui::TableHeadersRow();

        const uint32_t l_MaxRows =
            std::min<uint32_t>(16u,
                               static_cast<uint32_t>(l_Stats.size()));
        for (uint32_t i = 0; i < l_MaxRows; ++i) {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          ImGui::TextUnformatted(l_Stats[i].group.c_str());
          ImGui::TableSetColumnIndex(1);
          ImGui::TextUnformatted(l_Stats[i].name.c_str());
          ImGui::TableSetColumnIndex(2);
          ImGui::Text("%.3f", l_Stats[i].totalMs);
          ImGui::TableSetColumnIndex(3);
          ImGui::Text("%.3f", l_Stats[i].maxMs);
          ImGui::TableSetColumnIndex(4);
          ImGui::Text("%u", l_Stats[i].calls);
        }

        ImGui::EndTable();
      }
      ImGui::EndChild();
      ImGui::PopStyleColor(3);

      ImGui::Spacing();
      render_section_header(ICON_LC_PANEL_TOP, "Timeline");
      if (!l_Frames.empty()) {
        render_timeline(l_Frame);
      } else {
        ImGui::BeginChild("ProfilerTimelineEmpty",
                          ImVec2(0.0f, 120.0f), true);
        ImGui::PushStyleColor(ImGuiCol_Text,
                              color_to_imvec4(l_Theme.subtext));
        ImGui::TextUnformatted("Waiting for captured frames");
        ImGui::PopStyleColor();
        ImGui::EndChild();
      }

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low

#undef BUFFER_SIZE
