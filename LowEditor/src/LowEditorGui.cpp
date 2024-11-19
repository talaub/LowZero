#include "LowEditorGui.h"

#include "LowUtilHandle.h"

#include "LowCoreEntity.h"

#include "LowEditorThemes.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome5.h"
#include "IconsLucide.h"
#include <string.h>

#include "LowRendererImGuiHelper.h"

#include <algorithm>
#include <nfd.h>

namespace Low {
  namespace Editor {
    namespace Gui {

      static bool drag_float(const char *label, float *value,
                             float width, float v_speed = 1.0f,
                             float v_min = 0.0f, float v_max = 0.0f)
      {
        ImGui::PushID(
            label); // Ensure unique ID if using this multiple times

        // Get cursor position and input box size to draw the
        // background behind it
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        ImVec2 widget_size = ImVec2(
            width,
            ImGui::GetFrameHeight()); // Customize width as needed

        // Draw background with custom corner rounding
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        ImVec4 bg_color = color_to_imvec4(
            theme_get_current().input); // Customize color here
        draw_list->AddRectFilled(
            cursor_pos,
            ImVec2(cursor_pos.x + widget_size.x,
                   cursor_pos.y + widget_size.y),
            ImGui::GetColorU32(bg_color),
            2.0f,                         // Rounding amount
            ImDrawFlags_RoundCornersRight // Only round the right
                                          // corners
        );

        // Style the input itself to remove its own rounding and
        // padding
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,
                            0.0f); // Remove rounding from input
        ImGui::PushStyleColor(
            ImGuiCol_FrameBg,
            ImVec4(0, 0, 0,
                   0)); // Make input box background transparent

        // Draw the DragFloat on top of the custom background
        ImGui::SetCursorScreenPos(
            cursor_pos); // Reset cursor position
        ImGui::PushItemWidth(width);
        bool ret = ImGui::DragFloat("##value", value, v_speed, v_min,
                                    v_max, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();   // Restore rounding
        ImGui::PopStyleColor(); // Restore background color
        ImGui::PopID();

        return ret;
      }

      static bool input_text(const char *label, char *value,
                             int valuelength)
      {
        ImGui::PushID(
            label); // Ensure unique ID if using this multiple times

        // Get cursor position and input box size to draw the
        // background behind it
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        ImVec2 widget_size = ImVec2(
            200.0f,
            ImGui::GetFrameHeight()); // Customize width as needed

        // Draw background with custom corner rounding
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        ImVec4 bg_color = color_to_imvec4(
            theme_get_current().input); // Customize color here
        draw_list->AddRectFilled(
            cursor_pos,
            ImVec2(cursor_pos.x + widget_size.x,
                   cursor_pos.y + widget_size.y),
            ImGui::GetColorU32(bg_color),
            2.0f,                         // Rounding amount
            ImDrawFlags_RoundCornersRight // Only round the right
                                          // corners
        );

        // Style the input itself to remove its own rounding and
        // padding
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,
                            0.0f); // Remove rounding from input
        ImGui::PushStyleColor(
            ImGuiCol_FrameBg,
            ImVec4(0, 0, 0,
                   0)); // Make input box background transparent

        // Draw the DragFloat on top of the custom background
        ImGui::SetCursorScreenPos(
            cursor_pos); // Reset cursor position
        // ImGui::PushItemWidth(width);
        bool ret = ImGui::InputText("##value", value, valuelength);
        // ImGui::PopItemWidth();

        ImGui::PopStyleVar();   // Restore rounding
        ImGui::PopStyleColor(); // Restore background color
        ImGui::PopID();

        return ret;
      }

      static void draw_single_letter_label(
          Util::String p_Label, ImColor p_Color,
          float p_Width = 16.0f,
          ImVec2 p_IconOffset = ImVec2(0.0f, 0.0f))
      {
        using namespace ImGui;

        ImDrawList *l_DrawList = GetWindowDrawList();
        ImVec2 l_Pos = GetCursorScreenPos();

        /*
              l_DrawList->AddCircleFilled({l_Pos.x + l_CircleSize
           / 2.0f, l_Pos.y + (l_CircleSize / 2.0f)
           + 5.0f}, l_CircleSize, p_Color);
        */

        l_DrawList->AddRectFilled(l_Pos, l_Pos + ImVec2(p_Width, 23),
                                  p_Color, 2.0f,
                                  ImDrawFlags_RoundCornersLeft);

        SetCursorScreenPos({l_Pos.x + 3.0f + p_IconOffset.x, l_Pos.y + 2.0f + p_IconOffset.y} 
                           );
        Text(p_Label.c_str());
        SetCursorScreenPos({l_Pos.x + p_Width - 1.0f, l_Pos.y});
      }

      static bool draw_single_coefficient_editor(
          Util::String p_Label, ImColor p_Color, float *p_Value,
          float p_Width, float p_Margin, bool p_Break)
      {
        using namespace ImGui;

        ImVec2 l_Pos = GetCursorScreenPos();
        draw_single_letter_label(p_Label, p_Color);

        bool l_Changed = false;

        Util::String l_Label = "##" + p_Label;

        if (drag_float(l_Label.c_str(), p_Value, (p_Width - 16.0f),
                       0.2f, 0.0f, 0.0f)) {
          l_Changed = true;
        }

        if (!p_Break) {
          SetCursorScreenPos({l_Pos.x + p_Width + p_Margin, l_Pos.y});
        }

        return l_Changed;
      }

      static bool draw_single_coefficient_editor(Util::String p_Label,
                                                 ImColor p_Color,
                                                 float *p_Value,
                                                 float p_Width,
                                                 bool p_Break)
      {
        return draw_single_coefficient_editor(
            p_Label, p_Color, p_Value, p_Width, 0.0f, p_Break);
      }

      static bool draw_single_coefficient_editor(Util::String p_Label,
                                                 ImColor p_Color,
                                                 float *p_Value,
                                                 float p_Width,
                                                 float p_Margin)
      {
        return draw_single_coefficient_editor(
            p_Label, p_Color, p_Value, p_Width, p_Margin, false);
      }

      static bool draw_single_coefficient_editor(Util::String p_Label,
                                                 ImColor p_Color,
                                                 float *p_Value,
                                                 float p_Width)
      {
        return draw_single_coefficient_editor(p_Label, p_Color,
                                              p_Value, p_Width, true);
      }

      bool Vector3Edit(Math::Vector3 &p_Vector, float p_MaxWidth)
      {
        float l_FullWidth = ImGui::GetContentRegionAvail().x;
        if (p_MaxWidth > 0.0f) {
          l_FullWidth = p_MaxWidth;
        }
        float l_Spacing = LOW_EDITOR_SPACING;
        float l_Width = (l_FullWidth - (l_Spacing * 2.0f)) / 3.0f;
        bool l_Changed = false;

        Theme &l_Theme = theme_get_current();

        if (draw_single_coefficient_editor(
                "X", color_to_imcolor(l_Theme.coords0), &p_Vector.x,
                l_Width, l_Spacing)) {
          l_Changed = true;
        }
        if (draw_single_coefficient_editor(
                "Y", color_to_imcolor(l_Theme.coords1), &p_Vector.y,
                l_Width, l_Spacing)) {
          l_Changed = true;
        }
        if (draw_single_coefficient_editor(
                "Z", color_to_imcolor(l_Theme.coords2), &p_Vector.z,
                l_Width)) {
          l_Changed = true;
        }

        return l_Changed;
      }

      bool Vector2Edit(Math::Vector2 &p_Vector)
      {
        float l_FullWidth = ImGui::GetContentRegionAvail().x;
        float l_Spacing = LOW_EDITOR_SPACING;
        float l_Width = (l_FullWidth - (l_Spacing * 2.0f)) / 2.0f;
        bool l_Changed = false;

        if (draw_single_coefficient_editor(
                "X", IM_COL32(204, 42, 54, 255), &p_Vector.x, l_Width,
                l_Spacing)) {
          l_Changed = true;
        }
        if (draw_single_coefficient_editor("Y",
                                           IM_COL32(42, 204, 54, 255),
                                           &p_Vector.y, l_Width)) {
          l_Changed = true;
        }

        return l_Changed;
      }

      Util::String FileExplorer()
      {
        nfdchar_t *outPath = NULL;
        nfdresult_t result = NFD_OpenDialog(NULL, NULL, &outPath);

        if (result == NFD_OKAY) {
          Util::String l_Output = outPath;
          free(outPath);

          return l_Output;
        } else if (result == NFD_CANCEL) {
        } else {
          LOW_LOG_ERROR << "File explorer encountered an error: "
                        << NFD_GetError() << LOW_LOG_END;
        }

        return "";
      }

      bool spinner(const char *label, float radius, int thickness,
                   Math::Color p_Color)
      {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
          return false;

        ImGuiContext &g = *GImGui;
        const ImGuiStyle &style = g.Style;
        const ImGuiID id = window->GetID(label);

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size((radius) * 2,
                    (radius + style.FramePadding.y) * 2);

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id))
          return false;

        // Render
        window->DrawList->PathClear();

        int num_segments = 30;
        int start = abs(ImSin(g.Time * 1.8f) * (num_segments - 5));

        const float a_min =
            IM_PI * 2.0f * ((float)start) / (float)num_segments;
        const float a_max = IM_PI * 2.0f * ((float)num_segments - 3) /
                            (float)num_segments;

        const ImVec2 centre = ImVec2(
            pos.x + radius, pos.y + radius + style.FramePadding.y);

        for (int i = 0; i < num_segments; i++) {
          const float a = a_min + ((float)i / (float)num_segments) *
                                      (a_max - a_min);
          window->DrawList->PathLineTo(
              ImVec2(centre.x + ImCos(a + g.Time * 8) * radius,
                     centre.y + ImSin(a + g.Time * 8) * radius));
        }

        window->DrawList->PathStroke(
            IM_COL32(p_Color.x * 255, p_Color.y * 255,
                     p_Color.z * 255, p_Color.a * 255),
            false, thickness);
      }

      void drag_handle(Util::Handle p_Handle)
      {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
          uint64_t l_Id = p_Handle.get_id();
          ImGui::SetDragDropPayload("DG_HANDLE", &l_Id, sizeof(l_Id));
          Util::Name l_Name;
          Util::RTTI::TypeInfo &l_TypeInfo =
              Util::Handle::get_type_info(p_Handle.get_type());

          if (l_TypeInfo.component) {
            Core::Entity l_Entity =
                *(uint64_t *)l_TypeInfo.properties[N(entity)].get(
                    p_Handle);
            l_Name = l_Entity.get_name();
          } else {
            l_Name =
                *(Util::Name *)l_TypeInfo.properties[N(name)].get(
                    p_Handle);
          }

          ImGui::Text(l_Name.c_str());

          ImGui::EndDragDropSource();
        }
      }

      static bool draw_text_field_with_icon(
          Util::String p_Label, Util::String p_Icon, ImColor p_Color,
          char *p_Value, int p_Length, ImVec2 p_IconOffset)
      {
        using namespace ImGui;

        ImVec2 l_Pos = GetCursorScreenPos();
        ImGui::PushFont(Renderer::ImGuiHelper::fonts().lucide_400);
        draw_single_letter_label(p_Icon, p_Color, 22.0f,
                                 p_IconOffset);
        ImGui::PopFont();

        bool l_Changed = false;

        Util::String l_Label = "##" + p_Label;

        if (input_text(l_Label.c_str(), p_Value, p_Length)) {
          l_Changed = true;
        }

        // if (!p_Break) {
        // SetCursorScreenPos({l_Pos.x + p_Width + p_Margin,
        // l_Pos.y});
        //}

        return l_Changed;
      }

      bool SearchField(Util::String p_Label, char *p_SearchString,
                       int p_Length, ImVec2 p_IconOffset)
      {
        Theme &l_Theme = theme_get_current();

        return draw_text_field_with_icon(
            p_Label, ICON_LC_SEARCH,
            color_to_imcolor(l_Theme.buttonHover), p_SearchString,
            p_Length, p_IconOffset);
      }
    } // namespace Gui
  }   // namespace Editor
} // namespace Low
