#include "LowEditorThemes.h"

#include "LowUtil.h"
#include "LowUtilLogger.h"
#include "LowUtilFileIO.h"
#include "LowUtilContainers.h"
#include "LowUtilYaml.h"
#include "LowUtilSerialization.h"
#include "LowUtilString.h"
#include "LowMath.h"

#include <filesystem>
#include <iomanip>
#include <sstream>

namespace Low {
  namespace Editor {

    struct ThemeInfo
    {
      Util::String path;
      bool editable = false;
    };

    Util::Map<Util::Name, Theme> g_Themes;
    Util::Map<Util::Name, ThemeInfo> g_ThemeInfos;
    Util::Name g_SelectedTheme;

    static Util::String get_theme_directory()
    {
      return Util::get_project().dataPath + "/editor/themes";
    }

    static Util::String get_user_theme_directory()
    {
      return get_theme_directory() + "/user";
    }

    static Util::String get_theme_file_path(Util::Name p_Name)
    {
      return get_user_theme_directory() + "/" + p_Name.c_str() +
             ".editortheme.yaml";
    }

    static void ensure_user_theme_directory()
    {
      std::filesystem::create_directories(
          get_user_theme_directory().c_str());
    }

    static Util::String get_theme_name_from_path(Util::String p_Path)
    {
      Util::String l_Normalized =
          Util::StringHelper::replace(p_Path, '\\', '/');
      const Util::String l_Ending = ".editortheme.yaml";

      size_t l_Start = l_Normalized.find_last_of('/');
      if (l_Start == Util::String::npos) {
        l_Start = 0;
      } else {
        ++l_Start;
      }

      Util::String l_FileName = l_Normalized.substr(l_Start);
      if (Util::StringHelper::ends_with(l_FileName, l_Ending)) {
        return l_FileName.substr(
            0, l_FileName.length() - l_Ending.length());
      }
      return l_FileName;
    }

    static Util::String color_to_hex(const Math::Color &p_Color)
    {
      auto l_ToByte = [](float p_Value) -> int {
        return (int)(Low::Math::Util::clamp(p_Value, 0.0f, 1.0f) *
                         255.0f +
                     0.5f);
      };

      std::stringstream l_Stream;
      l_Stream << std::hex << std::setfill('0') << std::nouppercase
               << std::setw(2) << l_ToByte(p_Color.r)
               << std::setw(2) << l_ToByte(p_Color.g)
               << std::setw(2) << l_ToByte(p_Color.b);
      if (l_ToByte(p_Color.a) != 255) {
        l_Stream << std::setw(2) << l_ToByte(p_Color.a);
      }
      return l_Stream.str().c_str();
    }

    static void append_color(Util::String &p_Output,
                             const char *p_Key,
                             const Math::Color &p_Color)
    {
      p_Output += p_Key;
      p_Output += ": ";
      p_Output += color_to_hex(p_Color);
      p_Output += "\n";
    }

    static Util::String serialize_theme(const Theme &p_Theme)
    {
      Util::String l_Output;
      append_color(l_Output, "base", p_Theme.base);
      append_color(l_Output, "text", p_Theme.text);
      append_color(l_Output, "subtext", p_Theme.subtext);
      append_color(l_Output, "text_disabled", p_Theme.textDisabled);
      append_color(l_Output, "button", p_Theme.button);
      append_color(l_Output, "button_hover", p_Theme.buttonHover);
      append_color(l_Output, "button_active", p_Theme.buttonActive);
      append_color(l_Output, "button_border", p_Theme.buttonBorder);
      append_color(l_Output, "header", p_Theme.header);
      append_color(l_Output, "header_hover", p_Theme.headerHover);
      append_color(l_Output, "header_active", p_Theme.headerActive);
      append_color(l_Output, "menubar", p_Theme.menubar);
      append_color(l_Output, "coords0", p_Theme.coords0);
      append_color(l_Output, "coords1", p_Theme.coords1);
      append_color(l_Output, "coords2", p_Theme.coords2);
      append_color(l_Output, "coords3", p_Theme.coords3);
      append_color(l_Output, "debug", p_Theme.debug);
      append_color(l_Output, "info", p_Theme.info);
      append_color(l_Output, "error", p_Theme.error);
      append_color(l_Output, "warning", p_Theme.warning);
      append_color(l_Output, "profile", p_Theme.profile);
      append_color(l_Output, "title_background",
                   p_Theme.titleBackground);
      append_color(l_Output, "title_background_active",
                   p_Theme.titleBackgroundActive);
      append_color(l_Output, "title_background_collapsed",
                   p_Theme.titleBackgroundCollapsed);
      append_color(l_Output, "scrollbar_background",
                   p_Theme.scrollbarBackground);
      append_color(l_Output, "scrollbar", p_Theme.scrollbar);
      append_color(l_Output, "scrollbar_hover",
                   p_Theme.scrollbarHover);
      append_color(l_Output, "scrollbar_active",
                   p_Theme.scrollbarActive);
      append_color(l_Output, "tab", p_Theme.tab);
      append_color(l_Output, "tab_hover", p_Theme.tabHover);
      append_color(l_Output, "tab_active", p_Theme.tabActive);
      append_color(l_Output, "tab_unfocused", p_Theme.tabUnfocused);
      append_color(l_Output, "tab_unfocused_active",
                   p_Theme.tabUnfocusedActive);
      append_color(l_Output, "input", p_Theme.input);
      append_color(l_Output, "input_hover", p_Theme.inputHover);
      append_color(l_Output, "input_active", p_Theme.inputActive);
      append_color(l_Output, "popup_background",
                   p_Theme.popupBackground);
      append_color(l_Output, "border", p_Theme.border);
      append_color(l_Output, "success", p_Theme.success);
      append_color(l_Output, "save", p_Theme.save);
      append_color(l_Output, "add", p_Theme.add);
      append_color(l_Output, "remove", p_Theme.remove);
      append_color(l_Output, "clear", p_Theme.clear);
      append_color(l_Output, "edit", p_Theme.edit);
      append_color(l_Output, "controller", p_Theme.controller);
      append_color(l_Output, "play", p_Theme.play);
      return l_Output;
    }

    Math::Color color_from_hex(const char *p_Hex)
    {
      using namespace Low::Math;
      // Helpers (local lambdas to avoid polluting namespace)
      auto l_is_hex = [](char c) -> bool {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
               (c >= 'A' && c <= 'F');
      };

      auto l_hex_val = [](char c) -> int {
        if (c >= '0' && c <= '9')
          return c - '0';
        if (c >= 'a' && c <= 'f')
          return 10 + (c - 'a');
        return 10 + (c - 'A');
      };

      auto l_to_float_8bit = [](int v) -> float {
        return (float)v / 255.0f;
      };

      auto l_to_float_4bit = [](int v) -> float {
        // 0..15 -> scale to 0..255 by duplicating the nibble (common
        // convention)
        return (float)((v << 4) | v) / 255.0f;
      };

      // Defensive defaults
      Vector4 l_result;
      l_result.x = 1.0f; // magenta (error-color)
      l_result.y = 0.0f;
      l_result.z = 1.0f;
      l_result.w = 1.0f;

      if (!p_Hex) {
        LOW_LOG_WARN << "color_from_hex: null input" << LOW_LOG_END;
        return l_result;
      }

      // Copy/scan once to:
      // - trim surrounding whitespace
      // - skip leading '#'
      // - validate hex chars and count
      const char *l_begin = p_Hex;
      while (*l_begin == ' ' || *l_begin == '\t' ||
             *l_begin == '\n' || *l_begin == '\r')
        ++l_begin;

      if (*l_begin == '#')
        ++l_begin;

      // Determine end, count only hex digits
      const char *l_it = l_begin;
      int l_hex_count = 0;
      while (*l_it != '\0' && *l_it != ' ' && *l_it != '\t' &&
             *l_it != '\n' && *l_it != '\r') {
        if (!l_is_hex(*l_it)) {
          LOW_LOG_WARN
              << "color_from_hex: non-hex character encountered: '"
              << *l_it << "'" << LOW_LOG_END;
          return l_result;
        }
        ++l_hex_count;
        ++l_it;
      }

      // Supported counts: 3 (RGB), 4 (RGBA), 6 (RRGGBB), 8 (RRGGBBAA)
      if (!(l_hex_count == 3 || l_hex_count == 4 ||
            l_hex_count == 6 || l_hex_count == 8)) {
        LOW_LOG_WARN << "color_from_hex: unsupported hex length "
                     << l_hex_count << " (expected 3, 4, 6, or 8)"
                     << LOW_LOG_END;
        return l_result;
      }

      // Parse
      // We’ll read components as 4-bit nibbles (for 3/4) or 8-bit
      // pairs (for 6/8).
      const char *l_ptr = l_begin;

      if (l_hex_count == 3 || l_hex_count == 4) {
        // #RGB or #RGBA (each char is 4 bits)
        int r = l_hex_val(*l_ptr++);
        int g = l_hex_val(*l_ptr++);
        int b = l_hex_val(*l_ptr++);
        int a = 0xF; // default alpha 1.0
        if (l_hex_count == 4) {
          a = l_hex_val(*l_ptr++);
        }

        l_result.x = l_to_float_4bit(r);
        l_result.y = l_to_float_4bit(g);
        l_result.z = l_to_float_4bit(b);
        l_result.w = l_to_float_4bit(a);
      } else {
        // #RRGGBB or #RRGGBBAA (each pair is 8 bits)
        auto l_read_byte = [&](int &l_out) {
          int hi = l_hex_val(*l_ptr++);
          int lo = l_hex_val(*l_ptr++);
          l_out = (hi << 4) | lo;
        };

        int r8, g8, b8;
        l_read_byte(r8);
        l_read_byte(g8);
        l_read_byte(b8);

        int a8 = 255; // default alpha 1.0
        if (l_hex_count == 8) {
          l_read_byte(a8);
        }

        l_result.x = l_to_float_8bit(r8);
        l_result.y = l_to_float_8bit(g8);
        l_result.z = l_to_float_8bit(b8);
        l_result.w = l_to_float_8bit(a8);
      }

      return l_result;
    }

    ImVec4 color_to_imvec4(const Math::Color p_Color)
    {
      ImVec4 c;
      c.x = p_Color.r;
      c.y = p_Color.g;
      c.z = p_Color.b;
      c.w = p_Color.a;

      return c;
    }

    ImColor make_imcolor(const float p_Red, const float p_Green,
                         const float p_Blue, const float p_Alpha)
    {
      ImColor c;
      c.Value.x = p_Red;
      c.Value.y = p_Green;
      c.Value.z = p_Blue;
      c.Value.w = p_Alpha;

      return c;
    }

    ImColor color_to_imcolor(const Math::Color p_Color)
    {
      return make_imcolor(p_Color.r, p_Color.g, p_Color.b, p_Color.a);
    }

    static Math::Color parse_color(Util::Yaml::Node p_Node)
    {
      Util::String l_HexString = p_Node.as<std::string>().c_str();
      return color_from_hex(l_HexString.c_str());
    }

    static void parse_theme(Util::String p_Name,
                            Util::Yaml::Node p_Node,
                            Util::String p_Path, bool p_Editable)
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
      if (p_Node["button_border"]) {
        l_Theme.buttonBorder = parse_color(p_Node["button_border"]);
      } else {
        l_Theme.buttonBorder = l_Theme.border;
      }
      if (p_Node["success"]) {
        l_Theme.success = parse_color(p_Node["success"]);
      } else {
        l_Theme.success = l_Theme.coords1;
      }
      if (p_Node["save"]) {
        l_Theme.save = parse_color(p_Node["save"]);
      } else {
        l_Theme.save = l_Theme.coords2;
      }
      if (p_Node["clear"]) {
        l_Theme.clear = parse_color(p_Node["clear"]);
      } else {
        l_Theme.clear = l_Theme.profile;
      }
      if (p_Node["remove"]) {
        l_Theme.remove = parse_color(p_Node["remove"]);
      } else {
        l_Theme.remove = l_Theme.error;
      }
      if (p_Node["add"]) {
        l_Theme.add = parse_color(p_Node["add"]);
      } else {
        l_Theme.add = l_Theme.success;
      }
      if (p_Node["edit"]) {
        l_Theme.edit = parse_color(p_Node["edit"]);
      } else {
        l_Theme.edit = l_Theme.warning;
      }
      if (p_Node["controller"]) {
        l_Theme.controller = parse_color(p_Node["controller"]);
      } else {
        l_Theme.controller = l_Theme.success;
      }
      if (p_Node["play"]) {
        l_Theme.play = parse_color(p_Node["play"]);
      } else {
        l_Theme.play = l_Theme.text;
      }

      g_Themes[l_Name] = l_Theme;
      g_ThemeInfos[l_Name] = {p_Path, p_Editable};
    }

    void themes_load()
    {
      g_Themes.clear();
      g_ThemeInfos.clear();

      Util::String l_ThemeDirectory = get_theme_directory();
      Util::String l_UserThemeDirectory = get_user_theme_directory();
      ensure_user_theme_directory();

      Util::String l_Ending = ".editortheme.yaml";

      bool l_First = true;

      Util::List<Util::String> l_FilePaths;
      Util::FileIO::list_directory(l_ThemeDirectory.c_str(),
                                   l_FilePaths);
      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::String i_Name = get_theme_name_from_path(i_Path);

          Util::Yaml::Node i_Node =
              Util::Yaml::load_file(i_Path.c_str());

          parse_theme(i_Name, i_Node, i_Path, false);

          // Apply first theme found
          if (l_First) {
            theme_apply(LOW_NAME(i_Name.c_str()));
            l_First = !l_First;
          }
        }
      }

      l_FilePaths.clear();
      Util::FileIO::list_directory(l_UserThemeDirectory.c_str(),
                                   l_FilePaths);
      for (Util::String &i_Path : l_FilePaths) {
        if (Util::StringHelper::ends_with(i_Path, l_Ending)) {
          Util::String i_Name = get_theme_name_from_path(i_Path);

          Util::Yaml::Node i_Node =
              Util::Yaml::load_file(i_Path.c_str());

          parse_theme(i_Name, i_Node, i_Path, true);

          if (l_First) {
            theme_apply(LOW_NAME(i_Name.c_str()));
            l_First = !l_First;
          }
        }
      }
    }

    Util::List<Util::Name> themes_get_names()
    {
      Util::List<Util::Name> l_Names;
      for (auto it = g_Themes.begin(); it != g_Themes.end(); ++it) {
        l_Names.push_back(it->first);
      }
      return l_Names;
    }

    bool theme_exists(Util::Name p_Name)
    {
      return g_Themes.find(p_Name) != g_Themes.end();
    }

    bool theme_is_editable(Util::Name p_Name)
    {
      auto it = g_ThemeInfos.find(p_Name);
      return it != g_ThemeInfos.end() && it->second.editable;
    }

    bool theme_save(Util::Name p_Name)
    {
      if (!theme_exists(p_Name) || !theme_is_editable(p_Name)) {
        return false;
      }

      ensure_user_theme_directory();
      ThemeInfo &l_Info = g_ThemeInfos[p_Name];
      if (l_Info.path.empty()) {
        l_Info.path = get_theme_file_path(p_Name);
      }

      Util::FileIO::File l_File =
          Util::FileIO::open(l_Info.path.c_str(),
                             Util::FileIO::FileMode::WRITE);
      Util::FileIO::write_sync(l_File, serialize_theme(g_Themes[p_Name]));
      Util::FileIO::close(l_File);
      return true;
    }

    bool theme_duplicate(Util::Name p_Source, Util::Name p_NewName)
    {
      if (!theme_exists(p_Source) || !p_NewName.is_valid() ||
          theme_exists(p_NewName)) {
        return false;
      }

      ensure_user_theme_directory();
      g_Themes[p_NewName] = g_Themes[p_Source];
      g_ThemeInfos[p_NewName] = {get_theme_file_path(p_NewName), true};
      theme_save(p_NewName);
      theme_apply(p_NewName);
      return true;
    }

    bool theme_delete(Util::Name p_Name)
    {
      if (!theme_exists(p_Name) || !theme_is_editable(p_Name)) {
        return false;
      }

      Util::String l_Path = g_ThemeInfos[p_Name].path;
      if (!l_Path.empty()) {
        Util::FileIO::delete_sync(l_Path.c_str());
      }

      bool l_WasSelected = p_Name == g_SelectedTheme;
      g_Themes.erase(p_Name);
      g_ThemeInfos.erase(p_Name);

      if (l_WasSelected && !g_Themes.empty()) {
        theme_apply(g_Themes.begin()->first);
      }
      return true;
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
      /*
      colors[ImGuiCol_Header] =
          color_to_imvec4(Math::Color{0.0f, 1.0f, 0.0f, 1.0f});
          */
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
