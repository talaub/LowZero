#include "LowEditorLogWidget.h"

#include "LowEditorThemes.h"

#include "LowUtilLogger.h"
#include "LowUtilContainers.h"
#include "LowUtilString.h"

#include "LowRendererImGuiHelper.h"

#include "imgui.h"
#include "IconsFontAwesome5.h"
#include "LowEditorGui.h"

namespace Low {
  namespace Editor {
    Util::Deque<Util::Log::LogEntry> g_Entries;

    static const char *get_icon_for_level(uint8_t p_Level)
    {
      switch (p_Level) {
      case Util::Log::LogLevel::DEBUG:
        return ICON_FA_BUG;
      case Util::Log::LogLevel::PROFILE:
        return ICON_FA_STOPWATCH;
      case Util::Log::LogLevel::INFO:
        return ICON_FA_INFO;
      case Util::Log::LogLevel::WARN:
        return ICON_FA_EXCLAMATION_TRIANGLE;
      case Util::Log::LogLevel::ERROR:
        return ICON_FA_EXCLAMATION_CIRCLE;
      case Util::Log::LogLevel::FATAL:
        return ICON_FA_EXCLAMATION_TRIANGLE;
      default:
        return ICON_FA_BUG;
      }
    }

    static ImU32 get_color_for_level(uint8_t p_Level)
    {
      switch (p_Level) {
      case Util::Log::LogLevel::DEBUG:
        return color_to_imcolor(theme_get_current().debug);
      case Util::Log::LogLevel::PROFILE:
        return color_to_imcolor(theme_get_current().profile);
      case Util::Log::LogLevel::INFO:
        return color_to_imcolor(theme_get_current().info);
      case Util::Log::LogLevel::WARN:
        return color_to_imcolor(theme_get_current().warning);
      case Util::Log::LogLevel::ERROR:
        return color_to_imcolor(theme_get_current().error);
      case Util::Log::LogLevel::FATAL:
        return color_to_imcolor(theme_get_current().error);
      default:
        return color_to_imcolor(theme_get_current().text);
      }
    }

    static const char *
    get_module_label(const Util::Log::LogEntry &p_Entry)
    {
      if (p_Entry.module == "lowcore") {
        return "Engine";
      }
      if (p_Entry.module == "lowed") {
        return "Editor";
      }
      if (p_Entry.module == "misteda") {
        return "Game";
      }
      if (p_Entry.module == "scripting") {
        return "Script";
      }
      if (p_Entry.module == "flode") {
        return "Flode";
      }

      return "Low";
    }

    static void render_log_entry(const Util::Log::LogEntry &p_Entry)
    {
      ImGui::BeginGroup();
      ImGui::PushFont(Renderer::ImGuiHelper::fonts().icon_800);
      ImGui::PushStyleColor(ImGuiCol_Text,
                            get_color_for_level(p_Entry.level));
      ImGui::Text(get_icon_for_level(p_Entry.level));
      ImGui::PopStyleColor();
      ImGui::PopFont();
      ImGui::SameLine();
      ImGui::BeginGroup();
      ImGui::PushFont(Renderer::ImGuiHelper::fonts().common_500);
      ImGui::TextWrapped(p_Entry.message.c_str());
      ImGui::PopFont();

      ImGui::PushStyleColor(
          ImGuiCol_Text,
          color_to_imvec4(theme_get_current().subtext));
      ImGui::TextWrapped(get_module_label(p_Entry));
      ImGui::PopStyleColor();

      ImGui::EndGroup();
      ImGui::EndGroup();
      ImGui::Separator();
      ImGui::Spacing();
    }

    static bool apply_filter(const Util::Log::LogEntry &p_Entry)
    {
      if (p_Entry.module == "lowed") {
        return true;
      }
      if (p_Entry.module == "misteda") {
        return true;
      }
      if (p_Entry.module == "scripting") {
        return true;
      }
      if (p_Entry.module == "flode") {
        return true;
      }
      if (p_Entry.module == "lowcore" &&
          p_Entry.level != Util::Log::LogLevel::DEBUG) {
        return true;
      }

      return false;
    }

    void log_callback(const Util::Log::LogEntry &p_Entry)
    {
      if (!apply_filter(p_Entry)) {
        return;
      }

      if (g_Entries.size() > 40) {
        g_Entries.pop_front();
      }
      g_Entries.push_back(p_Entry);
    }

    void LogWidget::initialize()
    {
      Util::Log::register_log_callback(&log_callback);
    }

    void LogWidget::render(float p_Delta)
    {
      ImGui::Begin(ICON_FA_SCROLL " Log", &m_Open);

      if (ImGui::Button("Clear")) {
        g_Entries.clear();
      }
      ImGui::SameLine();

      static char l_Search[128] = "";
      ImGui::InputText("##searchinput", l_Search,
                       IM_ARRAYSIZE(l_Search));

      Low::Util::String l_SearchString = l_Search;
      l_SearchString.make_lower();

      ImGui::BeginChild("ChildL",
                        ImVec2(ImGui::GetContentRegionAvail().x,
                               ImGui::GetContentRegionAvail().y),
                        true, 0);

      for (auto it = g_Entries.begin(); it != g_Entries.end(); ++it) {
        Util::Log::LogEntry &i_Entry = *it;

        if (!l_SearchString.empty()) {
          Util::String i_EntryModule = get_module_label(i_Entry);
          i_EntryModule.make_lower();

          Util::String i_EntryMessage = i_Entry.message;
          i_EntryMessage.make_lower();

          bool i_Filtered = true;
          if (Util::StringHelper::contains(i_EntryModule,
                                           l_SearchString)) {
            i_Filtered = false;
          } else if (Util::StringHelper::contains(i_EntryMessage,
                                                  l_SearchString)) {
            i_Filtered = false;
          }

          if (i_Filtered) {
            continue;
          }
        }

        render_log_entry(i_Entry);
      }

      ImGui::EndChild();

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
