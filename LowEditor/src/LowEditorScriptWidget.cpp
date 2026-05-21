
#include "LowEditorScriptWidget.h"
#include "LowUtil.h"

#include "LowEditorIcons.h"
#include "LowEditorFonts.h"
#include "LowEditorGui.h"
#include "LowEditorThemes.h"
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

      init_editor();
    }

    void ScriptWidget::init_editor()
    {
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
      enforce_standard_mode();

      auto &l_Dis = m_Zep->GetDisplay();

      l_Dis.SetFont(
          Zep::ZepTextType::Text,
          std::make_shared<Zep::ZepFont_ImGui>(
              l_Dis, Fonts::Code(19.0f, Fonts::Weight::Regular), 19));
    }

    void ScriptWidget::enforce_standard_mode()
    {
      if (!m_Zep)
        return;

      m_Zep->GetEditor().SetGlobalMode(
          Zep::ZepMode_Standard::StaticName());
    }

    void ScriptWidget::load_file(Util::String p_Path)
    {
      m_Zep.reset();
      m_FilePath = p_Path.c_str();
      m_StatusText.clear();

      const Zep::NVec2f l_PixelScale(1.0f, 1.0f);
      m_Zep = std::make_unique<Zep::ZepEditor_ImGui>(
          fs::path(Util::get_project().dataPath.c_str()),
          l_PixelScale);

      auto &l_Ed = m_Zep->GetEditor();

      l_Ed.InitWithFile(p_Path.c_str());
      enforce_standard_mode();
    }

    void ScriptWidget::save_active_buffer()
    {
      if (!m_Zep)
        return;

      auto &l_Editor = m_Zep->GetEditor();
      Zep::ZepBuffer *l_Buffer = l_Editor.GetActiveBuffer();
      if (!l_Buffer)
        return;

      l_Editor.SaveBuffer(*l_Buffer);
      m_StatusText = l_Buffer->HasFileFlags(Zep::FileFlags::Dirty)
                         ? "Save failed"
                         : "Saved";
    }

    void ScriptWidget::reload_file()
    {
      if (m_FilePath.empty())
        return;

      load_file(m_FilePath.c_str());
      m_StatusText = "Reloaded";
    }

    void ScriptWidget::render_toolbar()
    {
      Zep::ZepBuffer *l_Buffer =
          m_Zep ? m_Zep->GetEditor().GetActiveBuffer() : nullptr;
      const bool l_HasBuffer = l_Buffer != nullptr;
      const bool l_IsDirty =
          l_HasBuffer &&
          l_Buffer->HasFileFlags(Zep::FileFlags::Dirty);
      const bool l_ReadOnly =
          l_HasBuffer &&
          (l_Buffer->HasFileFlags(Zep::FileFlags::ReadOnly) ||
           l_Buffer->HasFileFlags(Zep::FileFlags::Locked));

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 7));
      ImGui::BeginChild("##script_editor_toolbar", ImVec2(0, 44),
                        false,
                        ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoScrollWithMouse);

      if (Gui::SaveButton(!l_HasBuffer || l_ReadOnly)) {
        save_active_buffer();
      }

      ImGui::SameLine();
      if (Gui::Button("Reload", !l_HasBuffer || m_FilePath.empty(),
                      LOW_EDITOR_ICON_REFRESH,
                      theme_get_current().info)) {
        reload_file();
      }

      ImGui::SameLine();
      Gui::VerticalSeparator(8.0f);
      ImGui::SameLine();

      ImGui::AlignTextToFramePadding();
      if (l_ReadOnly) {
        ImGui::TextColored(ImVec4(0.75f, 0.74f, 0.84f, 1.0f),
                           LOW_EDITOR_ICON_FILE_LOCKED " Read-only");
      } else {
        ImGui::TextColored(l_IsDirty
                               ? ImVec4(0.95f, 0.72f, 0.20f, 1.0f)
                               : ImVec4(0.45f, 0.86f, 0.63f, 1.0f),
                           l_IsDirty
                               ? LOW_EDITOR_ICON_FILE_MODIFIED
                                     " Modified"
                               : LOW_EDITOR_ICON_FILE_SAVED " Saved");
      }

      ImGui::SameLine();
      Gui::VerticalSeparator(8.0f);
      ImGui::SameLine();

      ImGui::AlignTextToFramePadding();
      if (!m_FilePath.empty()) {
        ImGui::TextDisabled(
            LOW_EDITOR_ICON_SCRIPT " %s",
            fs::path(m_FilePath).filename().string().c_str());
      } else {
        ImGui::TextDisabled(LOW_EDITOR_ICON_SCRIPT " Untitled");
      }

      if (!m_StatusText.empty()) {
        ImGui::SameLine();
        ImGui::TextDisabled(LOW_EDITOR_ICON_STATUS_INFO " %s",
                            m_StatusText.c_str());
      }

      ImGui::EndChild();
      ImGui::PopStyleVar();
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
      render_toolbar();

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
        enforce_standard_mode();

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
