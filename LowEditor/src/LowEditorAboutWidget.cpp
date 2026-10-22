#include "LowEditorAboutWidget.h"

#include "LowEditorFonts.h"
#include "LowEditorGui.h"
#include "LowEditorThemes.h"

#include "LowRendererEditorImage.h"
#include "LowRendererResourceManager.h"

#include "LowUtil.h"
#include "LowUtilModuleInfo.h"
#include "LowUtilVersion.h"

#include "IconsLucide.h"
#include "imgui.h"

// Only sizes from the editor's font presets, others get baked
// mid-frame
#define ABOUT_FONT_TITLE 28.0f
#define ABOUT_FONT_BODY 14.0f
#define ABOUT_FONT_SECTION 12.0f

namespace Low {
  namespace Editor {
    static const ImVec2 g_AboutWindowSize(600.0f, 520.0f);
    static const float g_AboutLogoSize = 56.0f;
    static const float g_AboutLabelColumnWidth = 110.0f;
    static const float g_AboutFooterHeight = 44.0f;

    static const char *get_engine_source_label(Util::EngineSource p_Source)
    {
      switch (p_Source) {
      case Util::EngineSource::Packaged:
        return "Packaged";
      case Util::EngineSource::Source:
        return "Source build";
      default:
        return "Unknown";
      }
    }

    static void render_logo()
    {
      Renderer::EditorImage l_Logo =
          Renderer::EditorImage::find_by_name(N(lowlogo_90));

      if (l_Logo.is_alive() &&
          l_Logo.get_state() == Renderer::TextureState::UNLOADED) {
        Renderer::ResourceManager::load_editor_image(l_Logo);
      }

      const ImVec2 l_Size(g_AboutLogoSize, g_AboutLogoSize);
      if (l_Logo.is_alive() &&
          l_Logo.get_state() == Renderer::TextureState::LOADED &&
          l_Logo.get_gpu().is_imgui_texture_initialized()) {
        ImGui::Image(l_Logo.get_gpu().get_imgui_texture_id(), l_Size);
      } else {
        ImGui::Dummy(l_Size);
      }
    }

    static const ImVec2 g_AboutBadgePadding(8.0f, 3.0f);

    static float get_badge_width(const char *p_Label)
    {
      ImGui::PushFont(Fonts::UI(ABOUT_FONT_SECTION, Fonts::Weight::Bold));
      const float l_Width =
          ImGui::CalcTextSize(p_Label).x + g_AboutBadgePadding.x * 2.0f;
      ImGui::PopFont();
      return l_Width;
    }

    static void render_badge(const char *p_Label, Math::Color p_Color)
    {
      ImGui::PushFont(Fonts::UI(ABOUT_FONT_SECTION, Fonts::Weight::Bold));

      const ImVec2 l_Padding = g_AboutBadgePadding;
      const ImVec2 l_TextSize = ImGui::CalcTextSize(p_Label);
      const ImVec2 l_Min = ImGui::GetCursorScreenPos();
      const ImVec2 l_Max(l_Min.x + l_TextSize.x + l_Padding.x * 2.0f,
                         l_Min.y + l_TextSize.y + l_Padding.y * 2.0f);

      Math::Color l_Background = p_Color;
      l_Background.a = 0.15f;

      ImDrawList *l_DrawList = ImGui::GetWindowDrawList();
      l_DrawList->AddRectFilled(
          l_Min, l_Max,
          ImGui::GetColorU32(color_to_imvec4(l_Background)), 4.0f);
      l_DrawList->AddRect(l_Min, l_Max,
                          ImGui::GetColorU32(color_to_imvec4(p_Color)),
                          4.0f);
      l_DrawList->AddText(
          ImVec2(l_Min.x + l_Padding.x, l_Min.y + l_Padding.y),
          ImGui::GetColorU32(color_to_imvec4(p_Color)), p_Label);

      ImGui::Dummy(ImVec2(l_Max.x - l_Min.x, l_Max.y - l_Min.y));
      ImGui::PopFont();
    }

    static void render_section_header(const char *p_Icon,
                                      const char *p_Label)
    {
      const Theme &l_Theme = theme_get_current();

      ImGui::Dummy(ImVec2(0.0f, 10.0f));
      ImGui::PushFont(
          Fonts::UI(ABOUT_FONT_SECTION, Fonts::Weight::Bold));
      ImGui::PushStyleColor(ImGuiCol_Text,
                            color_to_imvec4(l_Theme.subtext));
      ImGui::TextUnformatted(p_Icon);
      ImGui::SameLine(0.0f, 6.0f);
      ImGui::TextUnformatted(p_Label);
      ImGui::PopStyleColor();
      ImGui::PopFont();

      const ImVec2 l_LineStart = ImGui::GetCursorScreenPos();
      ImGui::GetWindowDrawList()->AddLine(
          l_LineStart,
          ImVec2(l_LineStart.x + ImGui::GetContentRegionAvail().x,
                 l_LineStart.y),
          ImGui::GetColorU32(color_to_imvec4(l_Theme.border)));
      ImGui::Dummy(ImVec2(0.0f, 6.0f));
    }

    static bool begin_info_table(const char *p_Id)
    {
      if (!ImGui::BeginTable(p_Id, 2, ImGuiTableFlags_None)) {
        return false;
      }
      ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed,
                              g_AboutLabelColumnWidth);
      ImGui::TableSetupColumn("Value",
                              ImGuiTableColumnFlags_WidthStretch);
      return true;
    }

    static void render_info_row(const char *p_Label,
                                const Util::String &p_Value,
                                bool p_Monospace = false)
    {
      ImGui::TableNextRow();

      ImGui::TableSetColumnIndex(0);
      ImGui::PushStyleColor(
          ImGuiCol_Text, color_to_imvec4(theme_get_current().subtext));
      ImGui::TextUnformatted(p_Label);
      ImGui::PopStyleColor();

      ImGui::TableSetColumnIndex(1);
      if (p_Monospace) {
        ImGui::PushFont(Fonts::Code(ABOUT_FONT_BODY));
      }
      ImGui::TextWrapped("%s", p_Value.c_str());
      if (p_Monospace) {
        ImGui::PopFont();
      }
    }

    static void render_header()
    {
      const Util::Project &l_Project = Util::get_project();
      const Theme &l_Theme = theme_get_current();

      render_logo();
      ImGui::SameLine(0.0f, 14.0f);

      ImGui::BeginGroup();
      ImGui::PushFont(Fonts::UI(ABOUT_FONT_TITLE, Fonts::Weight::Bold));
      ImGui::TextUnformatted("LowEngine");
      ImGui::PopFont();

      ImGui::PushStyleColor(ImGuiCol_Text,
                            color_to_imvec4(l_Theme.subtext));
      ImGui::Text("Version %s", LOW_VERSION_STRING);
      ImGui::PopStyleColor();
      ImGui::EndGroup();

      if (l_Project.engineSource == Util::EngineSource::Source) {
        const char *l_BadgeLabel = ICON_LC_CODE " SOURCE BUILD";
        ImGui::SameLine();
        // Right-aligned
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                             ImGui::GetContentRegionAvail().x -
                             get_badge_width(l_BadgeLabel));
        render_badge(l_BadgeLabel, l_Theme.debug);
      }
    }

    static void render_engine_section()
    {
      const Util::Project &l_Project = Util::get_project();

      render_section_header(ICON_LC_CPU, "ENGINE");
      if (begin_info_table("##AboutEngine")) {
        render_info_row("Version", LOW_VERSION_STRING);
        render_info_row("Build", LOW_ENGINE_BUILD_ID, true);
        render_info_row("Origin",
                        get_engine_source_label(l_Project.engineSource));
        render_info_row("Location", l_Project.engineRootPath);
        ImGui::EndTable();
      }
    }

    static void render_project_section()
    {
      const Util::Project &l_Project = Util::get_project();

      render_section_header(ICON_LC_FOLDER, "PROJECT");
      if (begin_info_table("##AboutProject")) {
        render_info_row("Name", l_Project.name);
        render_info_row("Version", l_Project.version);
        render_info_row("Location", l_Project.rootPath);
        ImGui::EndTable();
      }
    }

    static void render_modules_section()
    {
      const Theme &l_Theme = theme_get_current();
      const Util::List<Util::RegisteredModuleInfo> &l_Modules =
          Util::get_module_infos();

      render_section_header(ICON_LC_PUZZLE, "MODULES");

      if (l_Modules.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text,
                              color_to_imvec4(l_Theme.textDisabled));
        ImGui::TextUnformatted("No modules loaded");
        ImGui::PopStyleColor();
        return;
      }

      const ImGuiTableFlags l_Flags = ImGuiTableFlags_RowBg |
                                      ImGuiTableFlags_BordersInnerH |
                                      ImGuiTableFlags_PadOuterX;
      if (!ImGui::BeginTable("##AboutModules", 4, l_Flags)) {
        return;
      }

      ImGui::TableSetupColumn("Module", ImGuiTableColumnFlags_WidthFixed,
                              100.0f);
      ImGui::TableSetupColumn("Version",
                              ImGuiTableColumnFlags_WidthFixed, 70.0f);
      ImGui::TableSetupColumn("Project build",
                              ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("Built against",
                              ImGuiTableColumnFlags_WidthStretch);

      ImGui::PushStyleColor(ImGuiCol_Text,
                            color_to_imvec4(l_Theme.subtext));
      ImGui::TableHeadersRow();
      ImGui::PopStyleColor();

      for (const Util::RegisteredModuleInfo &i_Module : l_Modules) {
        // Modules built against another engine build are the classic
        // stale-DLL problem in bundled projects
        const bool i_Mismatch =
            i_Module.engineBuildId != LOW_ENGINE_BUILD_ID;

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(i_Module.name.c_str());

        ImGui::TableSetColumnIndex(1);
        ImGui::TextUnformatted(i_Module.version.c_str());

        ImGui::PushFont(Fonts::Code(ABOUT_FONT_BODY));
        ImGui::TableSetColumnIndex(2);
        ImGui::TextUnformatted(i_Module.projectBuildId.c_str());

        ImGui::TableSetColumnIndex(3);
        if (i_Mismatch) {
          ImGui::PushStyleColor(ImGuiCol_Text,
                                color_to_imvec4(l_Theme.warning));
          ImGui::Text(ICON_LC_TRIANGLE_ALERT " %s",
                      i_Module.engineBuildId.c_str());
          ImGui::PopStyleColor();
        } else {
          ImGui::TextUnformatted(i_Module.engineBuildId.c_str());
        }
        ImGui::PopFont();
      }

      ImGui::EndTable();
    }

    // Plain text for bug reports
    static Util::String build_clipboard_text()
    {
      const Util::Project &l_Project = Util::get_project();

      Util::String l_Text;
      l_Text += "LowEngine ";
      l_Text += LOW_VERSION_STRING;
      l_Text += " (";
      l_Text += LOW_ENGINE_BUILD_ID;
      l_Text += ", ";
      l_Text += get_engine_source_label(l_Project.engineSource);
      l_Text += ")\n";

      l_Text += l_Project.name + " " + l_Project.version + "\n";

      for (const Util::RegisteredModuleInfo &i_Module :
           Util::get_module_infos()) {
        l_Text += "  " + i_Module.name + " " + i_Module.version + " (" +
                  i_Module.projectBuildId + ", built against " +
                  i_Module.engineVersion + " " +
                  i_Module.engineBuildId + ")\n";
      }

      return l_Text;
    }

    static void render_footer()
    {
      ImGui::Separator();
      ImGui::Dummy(ImVec2(0.0f, 2.0f));

      if (Gui::Button("Copy info", false, ICON_LC_COPY)) {
        ImGui::SetClipboardText(build_clipboard_text().c_str());
      }
    }

    void AboutWidget::render(float p_Delta)
    {
      ImGuiViewport *l_Viewport = ImGui::GetMainViewport();
      ImGui::SetNextWindowSize(g_AboutWindowSize, ImGuiCond_Always);
      ImGui::SetNextWindowPos(l_Viewport->GetCenter(),
                              ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

      const ImGuiWindowFlags l_Flags =
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
          ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings;

      if (ImGui::Begin(ICON_LC_INFO " About", &m_Open, l_Flags)) {
        ImGui::PushFont(Fonts::UI(ABOUT_FONT_BODY));

        // Footer stays put, the rest scrolls if many modules are loaded
        ImGui::BeginChild("##AboutContent",
                          ImVec2(0.0f, -g_AboutFooterHeight));
        render_header();
        render_engine_section();
        render_project_section();
        render_modules_section();
        ImGui::EndChild();

        render_footer();

        ImGui::PopFont();
      }
      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
