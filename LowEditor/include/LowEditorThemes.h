#pragma once

#include "LowEditorApi.h"

#include "LowUtilName.h"
#include "LowMath.h"

#include "imgui.h"

namespace Low {
  namespace Editor {
    struct Theme
    {
      Math::Color base;
      Math::Color text;
      Math::Color button;
      Math::Color buttonHover;
      Math::Color buttonActive;
      Math::Color buttonBorder;
      Math::Color header;
      Math::Color headerHover;
      Math::Color headerActive;
      Math::Color menubar;
      Math::Color coords0;
      Math::Color coords1;
      Math::Color coords2;
      Math::Color coords3;
      Math::Color subtext;
      Math::Color textDisabled;
      Math::Color debug;
      Math::Color warning;
      Math::Color info;
      Math::Color error;
      Math::Color profile;
      Math::Color titleBackground;
      Math::Color titleBackgroundActive;
      Math::Color titleBackgroundCollapsed;
      Math::Color scrollbarBackground;
      Math::Color scrollbar;
      Math::Color scrollbarHover;
      Math::Color scrollbarActive;
      Math::Color tab;
      Math::Color tabHover;
      Math::Color tabActive;
      Math::Color tabUnfocused;
      Math::Color tabUnfocusedActive;
      Math::Color input;
      Math::Color inputHover;
      Math::Color inputActive;
      Math::Color popupBackground;
      Math::Color border;
      Math::Color success;
      Math::Color save;
      Math::Color add;
      Math::Color remove;
      Math::Color clear;
      Math::Color edit;
    };

    void themes_load();

    bool themes_render_menu();

    bool theme_exists(Util::Name p_Name);
    void theme_apply(Util::Name p_Name);

    LOW_EDITOR_API Theme &theme_get_current();
    LOW_EDITOR_API Util::Name theme_get_current_name();

    LOW_EDITOR_API ImVec4 color_to_imvec4(Math::Color &p_Color);
    LOW_EDITOR_API ImColor color_to_imcolor(Math::Color &p_Color);
    LOW_EDITOR_API ImColor make_imcolor(const float p_Red,
                                        const float p_Green,
                                        const float p_Blue,
                                        const float p_Alpha = 1.0f);
    LOW_EDITOR_API Math::Color color_from_hex(const char *p_Hex);
  } // namespace Editor
} // namespace Low
