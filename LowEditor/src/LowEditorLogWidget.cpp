#include "LowEditorLogWidget.h"

#include "LowEditorFonts.h"
#include "LowEditorThemes.h"
#include "LowEditorGui.h"

#include "LowUtilLogger.h"
#include "LowUtilContainers.h"
#include "LowUtilString.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome5.h"
#include "IconsLucide.h"
#include "IconsCodicons.h"
#include "LowEditorGui.h"

namespace Low {
  namespace Editor {
    Util::Deque<Util::Log::LogEntry> g_Entries;

    static const char *get_icon_for_level(uint8_t p_Level)
    {
      switch (p_Level) {
      case Util::Log::LogLevel::DEBUG:
        return ICON_CI_BUG;
      case Util::Log::LogLevel::PROFILE:
        return ICON_LC_TIMER;
      case Util::Log::LogLevel::INFO:
        return ICON_CI_INFO;
      case Util::Log::LogLevel::WARN:
        return ICON_CI_WARNING;
      case Util::Log::LogLevel::ERROR:
        return ICON_CI_ERROR;
      case Util::Log::LogLevel::FATAL:
        return ICON_LC_SHIELD_ALERT;
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
        return "ENGINE";
      }
      if (p_Entry.module == "lowed") {
        return "EDITOR";
      }
      if (p_Entry.module == "misteda") {
        return "GAME";
      }
      if (p_Entry.module == "scripting") {
        return "SCRIPT";
      }
      if (p_Entry.module == "flode") {
        return "FLODE";
      }

      return "Low";
    }

    static void render_log_entry(const Util::Log::LogEntry &p_Entry,
                                 int p_Index)
    {
      ImGuiWindow *l_Window = ImGui::GetCurrentWindow();
      if (!l_Window || l_Window->SkipItems) {
        return;
      }

      ImDrawList *l_DrawList = l_Window->DrawList;

      // Start of row (before padding)
      const ImVec2 l_RowMin = ImGui::GetCursorScreenPos();
      const float l_Left = l_Window->WorkRect.Min.x;
      const float l_Right = l_Window->WorkRect.Max.x;

      l_DrawList->ChannelsSplit(2);
      l_DrawList->ChannelsSetCurrent(1);

      // --- CONTENT
      // -----------------------------------------------------
      ImGui::BeginGroup();

      // Icon padding
      const float l_IconPaddingTop =
          12.0f; // bigger push for large icon
      ImGui::SetCursorScreenPos(
          ImVec2(l_RowMin.x, l_RowMin.y + l_IconPaddingTop));

      ImGui::PushFont(Fonts::UI(40));
      ImGui::PushStyleColor(ImGuiCol_Text,
                            get_color_for_level(p_Entry.level));
      ImGui::Text(get_icon_for_level(p_Entry.level));
      ImGui::PopStyleColor();
      ImGui::PopFont();

      ImGui::SameLine();

      // Text block padding
      const float l_TextPaddingTop = 8.0f; // lighter push for text
      ImGui::SetCursorScreenPos(
          ImVec2(ImGui::GetCursorScreenPos().x,
                 l_RowMin.y + l_TextPaddingTop));

      ImGui::BeginGroup();
      {
        ImGui::PushFont(Fonts::UI());
        ImGui::TextWrapped(p_Entry.message.c_str());
        ImGui::PopFont();

        ImGui::PushFont(Fonts::UI(14, Fonts::Weight::Light));
        ImVec4 l_Color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        l_Color.w = 0.3f;
        ImGui::PushStyleColor(ImGuiCol_Text, l_Color);
        ImGui::TextWrapped(get_module_label(p_Entry));
        ImGui::PopStyleColor();
        ImGui::PopFont();
      }
      ImGui::EndGroup();

      ImGui::EndGroup();
      // ---------------------------------------------------------------

      const ImVec2 l_RowMax = ImGui::GetCursorScreenPos();

      // Background
      l_DrawList->ChannelsSetCurrent(0);
      if ((p_Index & 1) != 0) {
        const ImU32 l_BgCol =
            ImGui::GetColorU32(ImGuiCol_FrameBg, 0.28f);
        l_DrawList->AddRectFilled(ImVec2(l_Left, l_RowMin.y),
                                  ImVec2(l_Right, l_RowMax.y),
                                  l_BgCol);
      }

      // Separator
      {
        const ImU32 l_LineCol =
            ImGui::GetColorU32(ImGuiCol_Border, 0.25f);
        l_DrawList->AddLine(ImVec2(l_Left, l_RowMax.y),
                            ImVec2(l_Right, l_RowMax.y), l_LineCol,
                            1.0f);
      }

      l_DrawList->ChannelsMerge();
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

      /*
      LOW_LOG_DEBUG << "Test" << LOW_LOG_END;
      LOW_LOG_PROFILE << "Test" << LOW_LOG_END;
      LOW_LOG_INFO << "Test" << LOW_LOG_END;
      LOW_LOG_WARN << "Test" << LOW_LOG_END;
      LOW_LOG_ERROR << "Test" << LOW_LOG_END;
      LOW_LOG_FATAL << "Test" << LOW_LOG_END;
      */
    }

    void LogWidget::render(float p_Delta)
    {
      ImGui::Begin(ICON_LC_MESSAGE_SQUARE_WARNING " Log", &m_Open);

      if (Gui::ClearButton()) {
        g_Entries.clear();
      }
      ImGui::SameLine();

      static char l_Search[128] = "";
      Gui::SearchField("##searchinput", l_Search,
                       IM_ARRAYSIZE(l_Search), {0.0f, 3.0f});

      ImGui::Dummy({0.0f, 3.0f});

      Low::Util::String l_SearchString = l_Search;
      l_SearchString.make_lower();

      ImGui::BeginChild("ChildL",
                        ImVec2(ImGui::GetContentRegionAvail().x,
                               ImGui::GetContentRegionAvail().y),
                        true, 0);

      u32 l_Index = 0;
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

        render_log_entry(i_Entry, l_Index);
        l_Index++;
      }

      ImGui::EndChild();

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
