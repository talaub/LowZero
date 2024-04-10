#include "LowEditorThemes.h"

#include "LowUtil.h"
#include "LowUtilLogger.h"
#include "LowUtilFileIO.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"
#include "LowUtilString.h"
#include "LowMath.h"

namespace Low {
  namespace Editor {

    Util::Map<Util::Name, Theme> g_Themes;
    Util::Name g_SelectedTheme;

    ImVec4 color_to_imvec4(Math::Color &p_Color)
    {
      ImVec4 c;
      c.x = p_Color.r;
      c.y = p_Color.g;
      c.z = p_Color.b;
      c.w = p_Color.a;

      return c;
    }

    ImColor color_to_imcolor(Math::Color &p_Color)
    {
      ImColor c;
      c.Value.x = p_Color.r;
      c.Value.y = p_Color.g;
      c.Value.z = p_Color.b;
      c.Value.w = p_Color.a;

      return c;
    }

    static Math::Color parse_color(Util::Yaml::Node &p_Node)
    {
      Util::String l_HexString = LOW_YAML_AS_STRING(p_Node);
      std::string l_StdHexString = l_HexString.c_str();
      Math::Color l_Color;

      std::stringstream ss;
      ss << std::hex << l_StdHexString;
      unsigned int colorValue;
      ss >> colorValue;
      l_Color.r = ((colorValue >> 16) & 0xFF) / 255.0; // Red
      l_Color.g = ((colorValue >> 8) & 0xFF) / 255.0;  // Green
      l_Color.b = (colorValue & 0xFF) / 255.0;         // Blue

      l_Color.a = 1.0f;

      return l_Color;
    }

    static void parse_theme(Util::String p_Name,
                            Util::Yaml::Node p_Node)
    {
      Util::Name l_Name = LOW_NAME(p_Name.c_str());

      Theme l_Theme;
      if (p_Node["base"]) {
        l_Theme.base = parse_color(p_Node["base"]);
      }
      if (p_Node["text"]) {
        l_Theme.text = parse_color(p_Node["text"]);
      }
      if (p_Node["subtext"]) {
        l_Theme.subtext = parse_color(p_Node["subtext"]);
      }
      if (p_Node["text_disabled"]) {
        l_Theme.textDisabled = parse_color(p_Node["text_disabled"]);
      }
      if (p_Node["button"]) {
        l_Theme.button = parse_color(p_Node["button"]);
      }
      if (p_Node["button_hover"]) {
        l_Theme.buttonHover = parse_color(p_Node["button_hover"]);
      }
      if (p_Node["button_active"]) {
        l_Theme.buttonActive = parse_color(p_Node["button_active"]);
      }
      if (p_Node["header"]) {
        l_Theme.header = parse_color(p_Node["header"]);
      }
      if (p_Node["header_hover"]) {
        l_Theme.headerHover = parse_color(p_Node["header_hover"]);
      }
      if (p_Node["header_active"]) {
        l_Theme.headerActive = parse_color(p_Node["header_active"]);
      }
      if (p_Node["menubar"]) {
        l_Theme.menubar = parse_color(p_Node["menubar"]);
      }
      if (p_Node["coords0"]) {
        l_Theme.coords0 = parse_color(p_Node["coords0"]);
      }
      if (p_Node["coords1"]) {
        l_Theme.coords1 = parse_color(p_Node["coords1"]);
      }
      if (p_Node["coords2"]) {
        l_Theme.coords2 = parse_color(p_Node["coords2"]);
      }
      if (p_Node["coords3"]) {
        l_Theme.coords3 = parse_color(p_Node["coords3"]);
      }
      if (p_Node["debug"]) {
        l_Theme.debug = parse_color(p_Node["debug"]);
      }
      if (p_Node["info"]) {
        l_Theme.info = parse_color(p_Node["info"]);
      }
      if (p_Node["error"]) {
        l_Theme.error = parse_color(p_Node["error"]);
      }
      if (p_Node["warning"]) {
        l_Theme.warning = parse_color(p_Node["warning"]);
      }
      if (p_Node["profile"]) {
        l_Theme.profile = parse_color(p_Node["profile"]);
      }
      if (p_Node["title_background"]) {
        l_Theme.titleBackground =
            parse_color(p_Node["title_background"]);
      }
      if (p_Node["title_background_active"]) {
        l_Theme.titleBackgroundActive =
            parse_color(p_Node["title_background_active"]);
      }
      if (p_Node["title_background_collapsed"]) {
        l_Theme.titleBackgroundCollapsed =
            parse_color(p_Node["title_background_collapsed"]);
      }
      if (p_Node["scrollbar"]) {
        l_Theme.scrollbar = parse_color(p_Node["scrollbar"]);
      }
      if (p_Node["scrollbar_background"]) {
        l_Theme.scrollbarBackground =
            parse_color(p_Node["scrollbar_background"]);
      }
      if (p_Node["scrollbar_hover"]) {
        l_Theme.scrollbarHover =
            parse_color(p_Node["scrollbar_hover"]);
      }
      if (p_Node["scrollbar_active"]) {
        l_Theme.scrollbarActive =
            parse_color(p_Node["scrollbar_active"]);
      }
      if (p_Node["tab"]) {
        l_Theme.tab = parse_color(p_Node["tab"]);
      }
      if (p_Node["tab_hover"]) {
        l_Theme.tabHover = parse_color(p_Node["tab_hover"]);
      }
      if (p_Node["tab_active"]) {
        l_Theme.tabActive = parse_color(p_Node["tab_active"]);
      }
      if (p_Node["tab_unfocused"]) {
        l_Theme.tabUnfocused = parse_color(p_Node["tab_unfocused"]);
      }
      if (p_Node["tab_unfocused_active"]) {
        l_Theme.tabUnfocusedActive =
            parse_color(p_Node["tab_unfocused_active"]);
      }
      if (p_Node["input"]) {
        l_Theme.input = parse_color(p_Node["input"]);
      }
      if (p_Node["input_hover"]) {
        l_Theme.inputHover = parse_color(p_Node["input_hover"]);
      }
      if (p_Node["input_active"]) {
        l_Theme.inputActive = parse_color(p_Node["input_active"]);
      }
      if (p_Node["popup_background"]) {
        l_Theme.popupBackground =
            parse_color(p_Node["popup_background"]);
      }
      if (p_Node["border"]) {
        l_Theme.border = parse_color(p_Node["border"]);
      }

      g_Themes[l_Name] = l_Theme;
    }

    void themes_load()
    {
      Util::String l_ThemeDirectory =
          Util::get_project().dataPath + "/editor/themes";
      Util::List<Util::String> l_FilePaths;
      Util::FileIO::list_directory(l_ThemeDirectory.c_str(),
                                   l_FilePaths);
      Util::String l_Ending = ".editortheme.yaml";

      bool l_First = true;

      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::String i_Filename = i_Path.substr(
              l_ThemeDirectory.length() + 1, i_Path.length());
          Util::String i_Name = i_Filename.substr(
              0, i_Filename.length() - l_Ending.length());

          Util::Yaml::Node i_Node =
              Util::Yaml::load_file(i_Path.c_str());

          parse_theme(i_Name, i_Node);

          // Apply first theme found
          if (l_First) {
            theme_apply(LOW_NAME(i_Name.c_str()));
            l_First = !l_First;
          }
        }
      }
    }

    bool theme_exists(Util::Name p_Name)
    {
      return g_Themes.find(p_Name) != g_Themes.end();
    }

    void theme_apply(Util::Name p_Name)
    {
      if (!theme_exists(p_Name)) {
        LOW_LOG_ERROR << "Cannot find theme with name " << p_Name
                      << LOW_LOG_END;
        return;
      }

      g_SelectedTheme = p_Name;

      Theme &l_Theme = g_Themes[p_Name];

      ImVec4 *colors = ImGui::GetStyle().Colors;
      colors[ImGuiCol_Text] = color_to_imvec4(l_Theme.text);
      colors[ImGuiCol_WindowBg] = color_to_imvec4(l_Theme.base);
      colors[ImGuiCol_Button] = color_to_imvec4(l_Theme.button);
      colors[ImGuiCol_ButtonHovered] =
          color_to_imvec4(l_Theme.buttonHover);
      colors[ImGuiCol_ButtonActive] =
          color_to_imvec4(l_Theme.buttonActive);
      colors[ImGuiCol_Header] = color_to_imvec4(l_Theme.header);
      colors[ImGuiCol_HeaderHovered] =
          color_to_imvec4(l_Theme.headerHover);
      colors[ImGuiCol_HeaderActive] =
          color_to_imvec4(l_Theme.headerActive);
      colors[ImGuiCol_MenuBarBg] = color_to_imvec4(l_Theme.menubar);
      colors[ImGuiCol_TitleBg] =
          color_to_imvec4(l_Theme.titleBackground);
      colors[ImGuiCol_CheckMark] = colors[ImGuiCol_Text];
      colors[ImGuiCol_TitleBg] =
          color_to_imvec4(l_Theme.titleBackground);
      colors[ImGuiCol_TitleBgCollapsed] =
          color_to_imvec4(l_Theme.titleBackgroundCollapsed);
      colors[ImGuiCol_TitleBgActive] =
          color_to_imvec4(l_Theme.titleBackgroundActive);
      colors[ImGuiCol_ScrollbarBg] =
          color_to_imvec4(l_Theme.scrollbarBackground);
      colors[ImGuiCol_ScrollbarGrab] =
          color_to_imvec4(l_Theme.scrollbar);
      colors[ImGuiCol_ScrollbarGrabHovered] =
          color_to_imvec4(l_Theme.scrollbarHover);
      colors[ImGuiCol_ScrollbarGrabActive] =
          color_to_imvec4(l_Theme.scrollbarActive);
      colors[ImGuiCol_Tab] = color_to_imvec4(l_Theme.tab);
      colors[ImGuiCol_TabHovered] = color_to_imvec4(l_Theme.tabHover);
      colors[ImGuiCol_TabActive] = color_to_imvec4(l_Theme.tabActive);
      colors[ImGuiCol_TabUnfocused] =
          color_to_imvec4(l_Theme.tabUnfocused);
      colors[ImGuiCol_TabUnfocusedActive] =
          color_to_imvec4(l_Theme.tabUnfocusedActive);
      colors[ImGuiCol_FrameBg] = color_to_imvec4(l_Theme.input);
      colors[ImGuiCol_FrameBgHovered] =
          color_to_imvec4(l_Theme.inputHover);
      colors[ImGuiCol_FrameBgActive] =
          color_to_imvec4(l_Theme.inputActive);
      colors[ImGuiCol_PopupBg] =
          color_to_imvec4(l_Theme.popupBackground);
      colors[ImGuiCol_Border] = color_to_imvec4(l_Theme.border);
      colors[ImGuiCol_TextDisabled] =
          color_to_imvec4(l_Theme.textDisabled);
      // ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    }

    bool themes_render_menu()
    {
      bool l_Out = false;
      for (auto it = g_Themes.begin(); it != g_Themes.end(); ++it) {
        if (ImGui::MenuItem(it->first.c_str(), nullptr,
                            it->first == g_SelectedTheme)) {
          theme_apply(it->first);
          l_Out = true;
        }
      }

      return l_Out;
    }

    Theme &theme_get_current()
    {
      return g_Themes[g_SelectedTheme];
    }

    Util::Name theme_get_current_name()
    {
      return g_SelectedTheme;
    }
  } // namespace Editor
} // namespace Low
