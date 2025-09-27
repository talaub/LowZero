
#include "LowEditorScriptWidget.h"
#include "LowUtil.h"

#include "LowEditorIcons.h"
#include "LowEditorFonts.h"
#include "imgui.h"

#define ZEP_SINGLE_HEADER_BUILD
#include <zep.h>
#include <zep/imgui/editor_imgui.h>

#include <filesystem>
namespace fs = std::filesystem;

namespace Low {
  namespace Editor {
    ScriptWidget::ScriptWidget()
    {
      if (m_Zep)
        return;

      // Minimal ctor for this fork: root + pixel scale (often
      // ignored by the frontend)
      const Zep::NVec2f l_PixelScale(1.0f, 1.0f);
      m_Zep = std::make_unique<Zep::ZepEditor_ImGui>(
          fs::path(Util::get_project().dataPath.c_str()),
          l_PixelScale);

      // Ensure there is a buffer to show.
      // Most forks expose InitWithText on the core editor; use that
      // first.
      auto &l_Ed = m_Zep->GetEditor();
      l_Ed.InitWithText("Untitled", "Script Editor");

      auto &l_Dis = m_Zep->GetDisplay();

      l_Dis.SetFont(
          Zep::ZepTextType::Text,
          std::make_shared<Zep::ZepFont_ImGui>(
              l_Dis, Fonts::Code(19.0f, Fonts::Weight::Regular), 19));
    }

    void ScriptWidget::load_file(Util::String p_Path)
    {
      m_Zep.reset();

      const Zep::NVec2f l_PixelScale(1.0f, 1.0f);
      m_Zep = std::make_unique<Zep::ZepEditor_ImGui>(
          fs::path(Util::get_project().dataPath.c_str()),
          l_PixelScale);

      auto &l_Ed = m_Zep->GetEditor();

      l_Ed.InitWithFile(p_Path.c_str());
    }

    void ScriptWidget::render(float p_Delta)
    {
      if (!m_Zep)
        return;

      // Give the docked window a sane minimum so content != 0x0 on
      // first layout frame
      ImGui::SetNextWindowSizeConstraints(ImVec2(200, 150),
                                          ImVec2(FLT_MAX, FLT_MAX));
      ImGui::SetNextWindowSize(ImVec2(720, 480),
                               ImGuiCond_FirstUseEver);

      if (!ImGui::Begin(ICON_LC_BRACES " Code Editor", &m_Open)) {
        ImGui::End();
        return;
      }

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
      const ImVec2 l_Pos = ImGui::GetCursorScreenPos();
      const ImVec2 l_Size = ImGui::GetContentRegionAvail();

      if (l_Size.x >= 2.0f &&
          l_Size.y >=
              2.0f) // avoid transient 0x0 during docking reflow
      {
        const Zep::NRectf l_Rect(
            Zep::NVec2f(l_Pos.x, l_Pos.y),
            Zep::NVec2f(l_Pos.x + l_Size.x, l_Pos.y + l_Size.y));

        m_Zep->GetEditor().SetDisplayRegion(l_Rect.topLeftPx,
                                            l_Rect.bottomRightPx);

        m_Zep->HandleInput();

        ImFont *l_Code =
            Fonts::Code(/* size_px */ 19.0f, Fonts::Weight::Regular);

        ImGui::PushFont(l_Code);
        m_Zep->Display();
        ImGui::PopFont();
      }

      ImGui::PopStyleVar();
      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
