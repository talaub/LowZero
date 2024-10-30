#include "LowEditorScriptingErrorList.h"

#include "LowEditorThemes.h"

#include "imgui.h"

#include "LowUtilLogger.h"
#include "LowUtilContainers.h"

#include "LowRendererImGuiHelper.h"

#include "IconsFontAwesome5.h"
#include "IconsLucide.h"
#include "IconsCodicons.h"

namespace Low {
  namespace Editor {
    bool g_ErrorRegistered = false;
    bool g_CompilationRegistered = false;

    Util::List<ScriptingErrorList *>
        g_RegisteredGeneralErrorCallbacks;
    Util::List<ScriptingErrorList *> g_RegisteredCompilationCallbacks;

    void general_compilation_callback(
        const Core::Scripting::Compilation &p_Compilation)
    {
      for (auto it = g_RegisteredCompilationCallbacks.begin();
           it != g_RegisteredCompilationCallbacks.end();) {
        ScriptingErrorList *i_List = *it;

        if (i_List) {
          i_List->compilation_callback(p_Compilation);
          it++;
        } else {
          it = g_RegisteredCompilationCallbacks.erase(it);
        }
      }
    }

    void general_error_callback(const Core::Scripting::Error &p_Error)
    {
      for (auto it = g_RegisteredGeneralErrorCallbacks.begin();
           it != g_RegisteredGeneralErrorCallbacks.end();) {
        ScriptingErrorList *i_List = *it;

        if (i_List) {
          i_List->error_callback(p_Error);
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
        Core::Scripting::register_error_callback(
            &general_error_callback);
      }

      g_RegisteredGeneralErrorCallbacks.push_back(p_List);
    }

    static void
    register_compilation_callback(ScriptingErrorList *p_List)
    {
      if (!g_CompilationRegistered) {
        g_CompilationRegistered = true;
        Core::Scripting::register_compilation_callback(
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
      if (ImGui::Button("Clear")) {
        m_Errors.clear();
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
      for (u32 i = 0; i < m_Errors.size(); ++i) {
        Core::Scripting::Error &i_Error = m_Errors[i];

        if (!l_SearchString.empty()) {
          Util::String i_ErrorScriptName = i_Error.scriptName;
          i_ErrorScriptName.make_lower();

          Util::String i_ErrorMessage = i_Error.message;
          i_ErrorMessage.make_lower();

          bool i_Filtered = true;
          if (Util::StringHelper::contains(i_ErrorScriptName,
                                           l_SearchString)) {
            i_Filtered = false;
          } else if (Util::StringHelper::contains(i_ErrorMessage,
                                                  l_SearchString)) {
            i_Filtered = false;
          }

          if (i_Filtered) {
            continue;
          }
        }

        ImGui::BeginGroup();
        ImGui::PushFont(Renderer::ImGuiHelper::fonts().icon_800);
        ImGui::PushStyleColor(
            ImGuiCol_Text,
            color_to_imvec4(theme_get_current().error));
        ImGui::Text(ICON_CI_ERROR);
        ImGui::PopStyleColor();
        ImGui::PopFont();
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::PushFont(Renderer::ImGuiHelper::fonts().common_500);
        ImGui::TextWrapped(i_Error.message.c_str());
        ImGui::PopFont();

        ImGui::PushStyleColor(
            ImGuiCol_Text,
            color_to_imvec4(theme_get_current().subtext));
        Util::String i_Subtext = i_Error.scriptName;
        i_Subtext += " (";
        Util::StringHelper::append(i_Subtext, i_Error.line);
        i_Subtext += ")";
        ImGui::TextWrapped(i_Subtext.c_str());
        ImGui::PopStyleColor();

        ImGui::EndGroup();
        ImGui::EndGroup();
        ImGui::Separator();
        ImGui::Spacing();
      }
      ImGui::EndChild();
    }

    void ScriptingErrorList::error_callback(
        const Core::Scripting::Error &p_Error)
    {
      m_Errors.push_back(p_Error);
    }

    void ScriptingErrorList::compilation_callback(
        const Core::Scripting::Compilation &p_Compilation)
    {
      for (auto it = m_Errors.begin(); it != m_Errors.end();) {
        if (it->scriptPath == p_Compilation.scriptPath) {
          it = m_Errors.erase(it);
        } else {
          it++;
        }
      }
    }
  } // namespace Editor
} // namespace Low
