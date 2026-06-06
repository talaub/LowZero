#include "LowEditorScriptingErrorList.h"

#include "LowCoreScriptAsset.h"
#include "LowCoreScripting.h"
#include "LowEditorFonts.h"
#include "LowEditorThemes.h"
#include "LowEditorGui.h"

#include "LowCoreEventManager.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "LowUtilLogger.h"
#include "LowUtilContainers.h"
#include "LowUtilVariant.h"

#include "IconsFontAwesome5.h"
#include "IconsLucide.h"
#include "IconsCodicons.h"

namespace Low {
  namespace Editor {
    namespace {
      static const char *
      get_icon_for_type(Core::Scripting::EventType p_Type)
      {
        switch (p_Type) {
        case Core::Scripting::EventType::Info:
          return ICON_CI_INFO;
        case Core::Scripting::EventType::Warn:
          return ICON_CI_WARNING;
        case Core::Scripting::EventType::Error:
        default:
          return ICON_CI_ERROR;
        }
      }

      static const char *
      get_label_for_type(Core::Scripting::EventType p_Type)
      {
        switch (p_Type) {
        case Core::Scripting::EventType::Info:
          return "Info";
        case Core::Scripting::EventType::Warn:
          return "Warning";
        case Core::Scripting::EventType::Error:
        default:
          return "Error";
        }
      }

      static ImU32
      get_color_for_type(Core::Scripting::EventType p_Type)
      {
        const Theme &l_Theme = theme_get_current();
        switch (p_Type) {
        case Core::Scripting::EventType::Info:
          return (ImU32)color_to_imcolor(l_Theme.info);
        case Core::Scripting::EventType::Warn:
          return (ImU32)color_to_imcolor(l_Theme.warning);
        case Core::Scripting::EventType::Error:
        default:
          return (ImU32)color_to_imcolor(l_Theme.error);
        }
      }

      static Util::String
      get_script_label(const Core::Scripting::EventMessage &p_Message)
      {
        if (p_Message.script.is_alive()) {
          return p_Message.script.get_name().c_str();
        }

        return "Unknown script";
      }

      static Util::String get_location_label(
          const Core::Scripting::EventMessage &p_Message)
      {
        Util::String l_Label = "Line ";
        Util::StringHelper::append(l_Label, p_Message.row);
        if (p_Message.col > 0) {
          l_Label += ", Col ";
          Util::StringHelper::append(l_Label, p_Message.col);
        }
        return l_Label;
      }

      static bool message_matches_search(
          const Core::Scripting::EventMessage &p_Message,
          const Util::String &p_SearchString)
      {
        if (p_SearchString.empty()) {
          return true;
        }

        Util::String l_ScriptName = get_script_label(p_Message);
        l_ScriptName.make_lower();

        Util::String l_ErrorMessage = p_Message.msg;
        l_ErrorMessage.make_lower();

        Util::String l_Location = get_location_label(p_Message);
        l_Location.make_lower();

        return Util::StringHelper::contains(l_ScriptName,
                                            p_SearchString) ||
               Util::StringHelper::contains(l_ErrorMessage,
                                            p_SearchString) ||
               Util::StringHelper::contains(l_Location,
                                            p_SearchString);
      }

      static void render_empty_state(const char *p_Text,
                                     const char *p_Subtext)
      {
        const Theme &l_Theme = theme_get_current();
        ImDrawList *l_DrawList = ImGui::GetWindowDrawList();
        const ImVec2 l_Pos = ImGui::GetCursorScreenPos();
        const float l_Width = ImGui::GetContentRegionAvail().x;
        const float l_Height = 96.0f;

        l_DrawList->AddRectFilled(
            l_Pos, ImVec2(l_Pos.x + l_Width, l_Pos.y + l_Height),
            (ImU32)color_to_imcolor(l_Theme.input), 4.0f);
        l_DrawList->AddRect(
            l_Pos, ImVec2(l_Pos.x + l_Width, l_Pos.y + l_Height),
            (ImU32)color_to_imcolor(l_Theme.border), 4.0f);

        ImGui::SetCursorScreenPos(
            ImVec2(l_Pos.x + 14.0f, l_Pos.y + 15.0f));
        ImGui::PushFont(Fonts::UI(28));
        ImGui::PushStyleColor(ImGuiCol_Text,
                              color_to_imvec4(l_Theme.success));
        ImGui::TextUnformatted(ICON_LC_CIRCLE_CHECK);
        ImGui::PopStyleColor();
        ImGui::PopFont();

        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::PushFont(Fonts::UI());
        ImGui::TextUnformatted(p_Text);
        ImGui::PopFont();
        ImGui::PushStyleColor(ImGuiCol_Text,
                              color_to_imvec4(l_Theme.subtext));
        ImGui::TextWrapped(p_Subtext);
        ImGui::PopStyleColor();
        ImGui::EndGroup();

        ImGui::SetCursorScreenPos(
            ImVec2(l_Pos.x, l_Pos.y + l_Height));
        ImGui::Dummy(ImVec2(0.0f, 1.0f));
      }

      static void render_message_row(
          const Core::Scripting::EventMessage &p_Message, u32 p_Index)
      {
        ImGuiWindow *l_Window = ImGui::GetCurrentWindow();
        if (!l_Window || l_Window->SkipItems) {
          return;
        }

        ImDrawList *l_DrawList = l_Window->DrawList;
        const Theme &l_Theme = theme_get_current();
        const ImVec2 l_RowMin = ImGui::GetCursorScreenPos();
        const float l_Left = l_Window->WorkRect.Min.x;
        const float l_Right = l_Window->WorkRect.Max.x;
        const ImU32 l_Accent = get_color_for_type(p_Message.type);
        const float l_RowHeight = 50.0f;
        const ImVec2 l_RowMax(l_Right, l_RowMin.y + l_RowHeight);

        l_DrawList->ChannelsSplit(2);
        l_DrawList->ChannelsSetCurrent(0);
        const ImU32 l_Bg =
            (p_Index & 1) != 0
                ? ImGui::GetColorU32(ImGuiCol_FrameBg, 0.22f)
                : (ImU32)color_to_imcolor(l_Theme.base);
        l_DrawList->AddRectFilled(ImVec2(l_Left, l_RowMin.y),
                                  l_RowMax, l_Bg);
        l_DrawList->AddRectFilled(
            ImVec2(l_Left + 7.0f, l_RowMin.y + 6.0f),
            ImVec2(l_Left + 11.0f, l_RowMax.y - 6.0f), l_Accent, 2.0f,
            ImDrawFlags_RoundCornersLeft);
        l_DrawList->AddLine(
            ImVec2(l_Left, l_RowMax.y), ImVec2(l_Right, l_RowMax.y),
            ImGui::GetColorU32(ImGuiCol_Border, 0.25f), 1.0f);

        l_DrawList->ChannelsSetCurrent(1);

        const float l_IconX = l_Left + 20.0f;
        const float l_TextX = l_Left + 52.0f;
        const float l_TopY = l_RowMin.y + 7.0f;
        const float l_IconSize = 24.0f;
        const float l_IconY =
            l_RowMin.y + (l_RowHeight - l_IconSize) * 0.5f;
        l_DrawList->AddText(Fonts::UI(24), l_IconSize,
                            ImVec2(l_IconX, l_IconY), l_Accent,
                            get_icon_for_type(p_Message.type));

        const char *l_Message =
            p_Message.msg.empty() ? get_label_for_type(p_Message.type)
                                  : p_Message.msg.c_str();
        ImGui::PushClipRect(ImVec2(l_TextX, l_RowMin.y),
                            ImVec2(l_Right - 10.0f, l_RowMax.y),
                            true);
        l_DrawList->AddText(ImVec2(l_TextX, l_TopY),
                            ImGui::GetColorU32(ImGuiCol_Text),
                            l_Message);

        const ImU32 l_ScriptColor =
            (ImU32)color_to_imcolor(l_Theme.profile);
        const ImU32 l_SubtextColor =
            ImGui::GetColorU32(ImGuiCol_Text, 0.34f);
        const float l_SubY = l_TopY + 21.0f;
        ImFont *l_SubFont = Fonts::UI(14, Fonts::Weight::Light);
        const Util::String l_ScriptLabel =
            get_script_label(p_Message);
        l_DrawList->AddText(l_SubFont, 14.0f, ImVec2(l_TextX, l_SubY),
                            l_ScriptColor, ICON_CI_FILE_CODE);
        const float l_FileIconW =
            l_SubFont
                ->CalcTextSizeA(14.0f, 10000.0f, 0.0f,
                                ICON_CI_FILE_CODE)
                .x;
        l_DrawList->AddText(
            l_SubFont, 14.0f,
            ImVec2(l_TextX + l_FileIconW + 5.0f, l_SubY),
            l_ScriptColor, l_ScriptLabel.c_str());
        const float l_ScriptW =
            l_SubFont
                ->CalcTextSizeA(14.0f, 10000.0f, 0.0f,
                                l_ScriptLabel.c_str())
                .x;
        const Util::String l_LocationLabel =
            get_location_label(p_Message);
        l_DrawList->AddText(
            l_SubFont, 14.0f,
            ImVec2(l_TextX + l_FileIconW + l_ScriptW + 18.0f, l_SubY),
            l_SubtextColor, l_LocationLabel.c_str());
        ImGui::PopClipRect();

        l_DrawList->ChannelsMerge();
        ImGui::Dummy(ImVec2(0.0f, l_RowHeight));
      }
    } // namespace

    bool g_ErrorRegistered = false;
    bool g_CompilationRegistered = false;

    Util::List<ScriptingErrorList *>
        g_RegisteredGeneralErrorCallbacks;
    Util::List<ScriptingErrorList *> g_RegisteredCompilationCallbacks;

    void
    general_compilation_callback(Util::List<Util::Variant> &p_Params)
    {
      for (auto it = g_RegisteredCompilationCallbacks.begin();
           it != g_RegisteredCompilationCallbacks.end();) {
        ScriptingErrorList *i_List = *it;

        if (i_List) {
          i_List->compilation_callback(p_Params[0].as_u64());
          it++;
        } else {
          it = g_RegisteredCompilationCallbacks.erase(it);
        }
      }
    }

    void general_event_callback(Util::List<Util::Variant> &p_Params)
    {
      Core::Scripting::EventMessage l_Msg;
      l_Msg.type = (Core::Scripting::EventType)(u32)p_Params[0];
      l_Msg.row = p_Params[1];
      l_Msg.col = p_Params[2];
      l_Msg.msg = p_Params[3].as_string();
      l_Msg.script = p_Params[4].as_u64();

      for (auto it = g_RegisteredGeneralErrorCallbacks.begin();
           it != g_RegisteredGeneralErrorCallbacks.end();) {
        ScriptingErrorList *i_List = *it;

        if (i_List) {
          i_List->msg_callback(l_Msg);
          it++;
        } else {
          it = g_RegisteredGeneralErrorCallbacks.erase(it);
        }
      }
    }

    static void register_general_scripting_error_callback(
        ScriptingErrorList *p_List)
    {
      if (!g_ErrorRegistered) {
        g_ErrorRegistered = true;
        Core::EventManager::bind_event(N(LOW_SCRIPTING_MESSAGE),
                                       &general_event_callback);
      }

      g_RegisteredGeneralErrorCallbacks.push_back(p_List);
    }

    static void
    register_compilation_callback(ScriptingErrorList *p_List)
    {
      if (!g_CompilationRegistered) {
        g_CompilationRegistered = true;
        Core::EventManager::bind_event(N(LOW_SCRIPTING_COMPILE),
                                       &general_compilation_callback);
      }

      g_RegisteredCompilationCallbacks.push_back(p_List);
    }

    ScriptingErrorList::ScriptingErrorList()
    {
      register_general_scripting_error_callback(this);
      register_compilation_callback(this);
    }

    void ScriptingErrorList::render(float p_Delta)
    {
      (void)p_Delta;

      if (Gui::ClearButton()) {
        m_Messages.clear();
      }
      ImGui::SameLine();

      static char l_Search[128] = "";
      Gui::SearchField("##searchinput", l_Search,
                       IM_ARRAYSIZE(l_Search), {0.0f, 3.0f});

      Low::Util::String l_SearchString = l_Search;
      l_SearchString.make_lower();

      u32 l_VisibleCount = 0;
      for (u32 i = 0; i < m_Messages.size(); ++i) {
        if (message_matches_search(m_Messages[i], l_SearchString)) {
          ++l_VisibleCount;
        }
      }

      ImGui::Dummy({0.0f, 3.0f});

      ImGui::BeginChild("ChildL",
                        ImVec2(ImGui::GetContentRegionAvail().x,
                               ImGui::GetContentRegionAvail().y),
                        true, 0);

      if (m_Messages.empty()) {
        render_empty_state(
            "No scripting messages",
            "Compile output and AngelScript diagnostics "
            "will show up here.");
        ImGui::EndChild();
        return;
      }

      if (l_VisibleCount == 0) {
        render_empty_state(
            "No matching messages",
            "Try a different script name, message text, "
            "or line number.");
        ImGui::EndChild();
        return;
      }

      u32 l_RowIndex = 0;
      for (u32 i = 0; i < m_Messages.size(); ++i) {
        Core::Scripting::EventMessage &i_Message = m_Messages[i];

        if (!message_matches_search(i_Message, l_SearchString)) {
          continue;
        }

        render_message_row(i_Message, l_RowIndex);
        ++l_RowIndex;
      }
      ImGui::EndChild();
    }

    void ScriptingErrorList::msg_callback(
        const Core::Scripting::EventMessage &p_Message)
    {
      m_Messages.push_back(p_Message);
    }

    void ScriptingErrorList::compilation_callback(
        const Core::ScriptModule p_Module)
    {
      for (auto it = m_Messages.begin(); it != m_Messages.end();) {
        Core::ScriptAsset i_Script = it->script;
        if (i_Script.is_alive()) {
          if (i_Script.get_module() == p_Module) {
            it = m_Messages.erase(it);
          } else {
            it++;
          }
        } else {
          ++it;
        }
      }
    }
  } // namespace Editor
} // namespace Low
