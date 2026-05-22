#include "LowEditorThemeWidget.h"

#include "LowEditorGui.h"
#include "LowEditorPropertyEditors.h"
#include "LowEditorThemes.h"
#include "LowEditorIcons.h"

#include "LowUtilString.h"

#include <imgui.h>

namespace Low {
  namespace Editor {

    static bool render_theme_selector()
    {
      Util::List<Util::Name> l_Names = themes_get_names();
      Util::Name l_CurrentTheme = theme_get_current_name();
      bool l_Changed = false;

      PropertyEditors::render_line("Theme", [&]() -> bool {
        const char *l_CurrentName =
            l_CurrentTheme.is_valid() ? l_CurrentTheme.c_str() : "";

        if (ImGui::BeginCombo("##theme", l_CurrentName)) {
          for (Util::Name i_Name : l_Names) {
            const bool i_Selected = i_Name == l_CurrentTheme;
            if (ImGui::Selectable(i_Name.c_str(), i_Selected)) {
              theme_apply(i_Name);
              l_Changed = true;
            }
            if (i_Selected) {
              ImGui::SetItemDefaultFocus();
            }
          }
          ImGui::EndCombo();
        }

        return l_Changed;
      });

      return l_Changed;
    }

    static bool render_theme_color(Util::String p_Label,
                                   Math::Color &p_Color)
    {
      return PropertyEditors::render_line(
          p_Label, [&p_Label, &p_Color]() -> bool {
            Math::ColorRGB l_Color(p_Color.r, p_Color.g, p_Color.b);
            if (!Gui::ColorRGBInput(
                    (Util::String("##") + p_Label).c_str(),
                    &l_Color)) {
              return false;
            }

            p_Color.r = l_Color.r;
            p_Color.g = l_Color.g;
            p_Color.b = l_Color.b;
            return true;
          });
    }

    static void render_theme_editor(Theme &p_Theme)
    {
      bool l_Changed = false;

#define LOW_EDITOR_THEME_COLOR(NAME, FIELD)                          \
  l_Changed = render_theme_color(NAME, p_Theme.FIELD) || l_Changed

      LOW_EDITOR_THEME_COLOR("Base", base);
      LOW_EDITOR_THEME_COLOR("Text", text);
      LOW_EDITOR_THEME_COLOR("Subtext", subtext);
      LOW_EDITOR_THEME_COLOR("Text disabled", textDisabled);
      LOW_EDITOR_THEME_COLOR("Button", button);
      LOW_EDITOR_THEME_COLOR("Button hover", buttonHover);
      LOW_EDITOR_THEME_COLOR("Button active", buttonActive);
      LOW_EDITOR_THEME_COLOR("Button border", buttonBorder);
      LOW_EDITOR_THEME_COLOR("Header", header);
      LOW_EDITOR_THEME_COLOR("Header hover", headerHover);
      LOW_EDITOR_THEME_COLOR("Header active", headerActive);
      LOW_EDITOR_THEME_COLOR("Menubar", menubar);
      LOW_EDITOR_THEME_COLOR("Coords 0", coords0);
      LOW_EDITOR_THEME_COLOR("Coords 1", coords1);
      LOW_EDITOR_THEME_COLOR("Coords 2", coords2);
      LOW_EDITOR_THEME_COLOR("Coords 3", coords3);
      LOW_EDITOR_THEME_COLOR("Debug", debug);
      LOW_EDITOR_THEME_COLOR("Info", info);
      LOW_EDITOR_THEME_COLOR("Error", error);
      LOW_EDITOR_THEME_COLOR("Warning", warning);
      LOW_EDITOR_THEME_COLOR("Profile", profile);
      LOW_EDITOR_THEME_COLOR("Title background", titleBackground);
      LOW_EDITOR_THEME_COLOR("Title background active",
                             titleBackgroundActive);
      LOW_EDITOR_THEME_COLOR("Title background collapsed",
                             titleBackgroundCollapsed);
      LOW_EDITOR_THEME_COLOR("Scrollbar background",
                             scrollbarBackground);
      LOW_EDITOR_THEME_COLOR("Scrollbar", scrollbar);
      LOW_EDITOR_THEME_COLOR("Scrollbar hover", scrollbarHover);
      LOW_EDITOR_THEME_COLOR("Scrollbar active", scrollbarActive);
      LOW_EDITOR_THEME_COLOR("Tab", tab);
      LOW_EDITOR_THEME_COLOR("Tab hover", tabHover);
      LOW_EDITOR_THEME_COLOR("Tab active", tabActive);
      LOW_EDITOR_THEME_COLOR("Tab unfocused", tabUnfocused);
      LOW_EDITOR_THEME_COLOR("Tab unfocused active",
                             tabUnfocusedActive);
      LOW_EDITOR_THEME_COLOR("Input", input);
      LOW_EDITOR_THEME_COLOR("Input hover", inputHover);
      LOW_EDITOR_THEME_COLOR("Input active", inputActive);
      LOW_EDITOR_THEME_COLOR("Popup background", popupBackground);
      LOW_EDITOR_THEME_COLOR("Border", border);
      LOW_EDITOR_THEME_COLOR("Success", success);
      LOW_EDITOR_THEME_COLOR("Save", save);
      LOW_EDITOR_THEME_COLOR("Add", add);
      LOW_EDITOR_THEME_COLOR("Remove", remove);
      LOW_EDITOR_THEME_COLOR("Clear", clear);
      LOW_EDITOR_THEME_COLOR("Edit", edit);
      LOW_EDITOR_THEME_COLOR("Controller", controller);
      LOW_EDITOR_THEME_COLOR("Play", play);

#undef LOW_EDITOR_THEME_COLOR

      if (l_Changed) {
        Util::Name l_CurrentTheme = theme_get_current_name();
        theme_apply(l_CurrentTheme);
      }
    }

    void ThemeWidget::render(float p_Delta)
    {
      ImGui::Begin("Themes", &m_Open);

      static Util::String l_DuplicateName;

      Util::Name l_CurrentTheme = theme_get_current_name();
      if (l_DuplicateName.empty() && l_CurrentTheme.is_valid()) {
        l_DuplicateName = l_CurrentTheme.c_str();
        l_DuplicateName += "_copy";
      }

      render_theme_selector();
      l_CurrentTheme = theme_get_current_name();

      bool l_Editable = theme_is_editable(l_CurrentTheme);

      PropertyEditors::render_line("Duplicate name", [&]() -> bool {
        return Gui::InputText("duplicate_name", l_DuplicateName);
      });

      ImGui::Dummy(ImVec2(0, 5));

      if (Gui::AddButton("Duplicate")) {
        Util::String l_Name =
            Util::StringHelper::technify_string(l_DuplicateName);
        if (!l_Name.empty() &&
            theme_duplicate(l_CurrentTheme,
                            LOW_NAME(l_Name.c_str()))) {
          l_DuplicateName = l_Name;
          l_DuplicateName += "_copy";
        }
      }

      if (l_Editable) {
        ImGui::SameLine();
        if (Gui::SaveButton()) {
          theme_save(l_CurrentTheme);
        }

        ImGui::SameLine();
        if (Gui::DeleteButton()) {
          theme_delete(l_CurrentTheme);
        }
      }

      ImGui::Separator();

      Theme &l_Theme = theme_get_current();
      if (l_Editable) {
        render_theme_editor(l_Theme);
      } else {
        ImGui::TextUnformatted(
            "Built-in themes are read-only. Duplicate this theme to "
            "edit it.");
      }

      ImGui::End();
    }
  } // namespace Editor
} // namespace Low
