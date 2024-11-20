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

#define DRAG_BUTTON_WIDTH 27.0f

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

        SetCursorScreenPos({l_Pos.x + 3.0f + p_IconOffset.x,
                            l_Pos.y + 2.0f + p_IconOffset.y});
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
            p_Label, ICON_LC_SEARCH, color_to_imcolor(l_Theme.input),
            p_SearchString, p_Length, p_IconOffset);
      }

      bool Checkbox(const char *label, bool *v)
      {
        using namespace ImGui;

        ImGuiWindow *window = GetCurrentWindow();
        if (window->SkipItems)
          return false;

        ImGuiContext &g = *GImGui;
        const ImGuiStyle &style = g.Style;
        const ImGuiID id = window->GetID(label);
        const ImVec2 label_size = CalcTextSize(label, NULL, true);

        const ImRect check_bb(
            window->DC.CursorPos,
            window->DC.CursorPos +
                ImVec2(label_size.y + style.FramePadding.y * 0.5,
                       label_size.y + style.FramePadding.y * 0.5));
        ItemSize(check_bb, style.FramePadding.y);

        ImRect total_bb = check_bb;
        if (label_size.x > 0)
          SameLine(0, style.ItemInnerSpacing.x);
        const ImRect text_bb(
            window->DC.CursorPos + ImVec2(0, style.FramePadding.y) -
                ImVec2(0, 2),
            window->DC.CursorPos + ImVec2(0, style.FramePadding.y) +
                label_size);
        if (label_size.x > 0) {
          ItemSize(ImVec2(text_bb.GetWidth(), check_bb.GetHeight()),
                   style.FramePadding.y);
          total_bb = ImRect(ImMin(check_bb.Min, text_bb.Min),
                            ImMax(check_bb.Max, text_bb.Max));
        }

        if (!ItemAdd(total_bb, id))
          return false;

        bool hovered, held;
        bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
        if (pressed)
          *v = !(*v);

        RenderFrame(check_bb.Min, check_bb.Max,
                    GetColorU32((held && hovered)
                                    ? ImGuiCol_FrameBgActive
                                : hovered ? ImGuiCol_FrameBgHovered
                                          : ImGuiCol_FrameBg),
                    true, style.FrameRounding);
        if (*v) {
          const float check_sz =
              ImMin(check_bb.GetWidth(), check_bb.GetHeight());
          const float pad =
              ImMax(1.0f, (float)(int)(check_sz / 6.0f));
          const ImVec2 pts[] = {
              ImVec2{check_bb.Min.x + pad,
                     check_bb.Min.y +
                         ((check_bb.Max.y - check_bb.Min.y) / 2)},
              ImVec2{check_bb.Min.x +
                         ((check_bb.Max.x - check_bb.Min.x) / 3),
                     check_bb.Max.y - pad * 1.5f},
              ImVec2{check_bb.Max.x - pad, check_bb.Min.y + pad}};
          window->DrawList->AddPolyline(
              pts, 3, GetColorU32(ImGuiCol_CheckMark), false, 2.0f);
        }

        if (label_size.x > 0.0f)
          RenderText(text_bb.GetTL(), label);

        return pressed;
      }

      bool DragIntWithButtons(const char *label, int *value,
                              int speed, int min, int max)
      {
        // Start a new horizontal group
        ImGui::BeginGroup();

        bool l_Edited = false;

        float l_FullWidth = ImGui::GetContentRegionAvail().x;
        float l_ButtonWidth = DRAG_BUTTON_WIDTH;
        float l_DragWidth = l_FullWidth - (l_ButtonWidth * 3.0f);

        ImVec2 l_CursorStart = ImGui::GetCursorPos();

        // DragFloat for the main value control
        ImGui::PushID(label);
        ImGui::PushItemWidth(l_DragWidth + 3.0f);
        if (ImGui::DragInt("##Drag", value, speed, min, max)) {
          // Ensure value stays clamped to min/max bounds (if
          // provided)
          if (min < max) {
            *value = ImClamp(*value, min, max);
          }
          l_Edited = true;
        }
        ImGui::PopItemWidth();

        ImGui::SetCursorPos(l_CursorStart +
                            ImVec2(l_DragWidth, 0.0f));

        if (ButtonNoRounding(ICON_LC_PLUS)) {
          *value += 1.0f; // Increase by 1.0
          if (min < max) {
            *value = ImClamp(*value, min, max);
          }
          l_Edited = true;
        }

        ImGui::SetCursorPos(
            l_CursorStart +
            ImVec2(l_DragWidth + l_ButtonWidth, 0.0f));

        if (ButtonRounding(ICON_LC_MINUS,
                           ImDrawFlags_RoundCornersRight)) {
          *value -= 1.0f; // Decrease by 1.0
          if (min < max) {
            *value = ImClamp(*value, min, max);
          }
          l_Edited = true;
        }

        // End horizontal group
        ImGui::PopID();
        ImGui::EndGroup();

        return l_Edited;
      }

      bool DragFloatWithButtons(const char *label, float *value,
                                float speed, float min, float max,
                                const char *format)
      {
        // Start a new horizontal group
        ImGui::BeginGroup();

        bool l_Edited = false;

        float l_FullWidth = ImGui::GetContentRegionAvail().x;
        float l_ButtonWidth = DRAG_BUTTON_WIDTH;
        float l_DragWidth = l_FullWidth - (l_ButtonWidth * 3.0f);

        ImVec2 l_CursorStart = ImGui::GetCursorPos();

        // DragFloat for the main value control
        ImGui::PushID(label);
        ImGui::PushItemWidth(l_DragWidth + 3.0f);
        if (ImGui::DragFloat("##Drag", value, speed, min, max,
                             format)) {
          // Ensure value stays clamped to min/max bounds (if
          // provided)
          if (min < max) {
            *value = ImClamp(*value, min, max);
          }
          l_Edited = true;
        }
        ImGui::PopItemWidth();

        ImGui::SetCursorPos(l_CursorStart +
                            ImVec2(l_DragWidth, 0.0f));

        if (ButtonNoRounding(ICON_LC_PLUS)) {
          *value += 1.0f; // Increase by 1.0
          if (min < max) {
            *value = ImClamp(*value, min, max);
          }
          l_Edited = true;
        }

        ImGui::SetCursorPos(
            l_CursorStart +
            ImVec2(l_DragWidth + l_ButtonWidth, 0.0f));

        if (ButtonRounding(ICON_LC_MINUS,
                           ImDrawFlags_RoundCornersRight)) {
          *value -= 1.0f; // Decrease by 1.0
          if (min < max) {
            *value = ImClamp(*value, min, max);
          }
          l_Edited = true;
        }

        // End horizontal group
        ImGui::PopID();
        ImGui::EndGroup();

        return l_Edited;
      }

      bool ButtonNoRounding(const char *p_Text)
      {
        ImGuiStyle &style = ImGui::GetStyle();
        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        ImVec2 button_pos = ImGui::GetCursorScreenPos();
        ImVec2 button_size =
            ImVec2(ImGui::CalcTextSize(p_Text).x + 10.0f,
                   ImGui::GetFrameHeight());
        ImRect rect(button_pos, ImVec2(button_pos.x + button_size.x,
                                       button_pos.y + button_size.y));

        Util::String l_Label = "##";
        l_Label += p_Text;

        // Draw the button itself
        bool l_Pressed =
            ImGui::InvisibleButton(l_Label.c_str(), button_size);

        ImU32 l_ButtonColor = ImGui::GetColorU32(ImGuiCol_Button);
        if (ImGui::IsItemHovered()) {
          l_ButtonColor = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
        }

        // Draw the background
        draw_list->AddRectFilled(rect.Min, rect.Max, l_ButtonColor,
                                 0.0f);

        // Draw the text
        draw_list->AddText(
            ImVec2(button_pos.x + 5.0f,
                   button_pos.y + style.FramePadding.y),
            ImGui::GetColorU32(ImGuiCol_Text), p_Text);

        return l_Pressed;
      }

      bool ButtonRounding(const char *p_Text, u32 p_Flags)
      {
        ImGuiStyle &style = ImGui::GetStyle();
        ImDrawList *draw_list = ImGui::GetWindowDrawList();

        float original_rounding = style.FrameRounding;

        ImVec2 button_pos = ImGui::GetCursorScreenPos();
        ImVec2 button_size =
            ImVec2(ImGui::CalcTextSize(p_Text).x + 10.0f,
                   ImGui::GetFrameHeight());
        ImRect rect(button_pos, ImVec2(button_pos.x + button_size.x,
                                       button_pos.y + button_size.y));

        Util::String l_Label = "##";
        l_Label += p_Text;

        // Draw the button itself
        bool l_Pressed =
            ImGui::InvisibleButton(l_Label.c_str(), button_size);

        ImU32 l_ButtonColor = ImGui::GetColorU32(ImGuiCol_Button);
        if (ImGui::IsItemHovered()) {
          l_ButtonColor = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
        }

        // Draw the background
        draw_list->AddRectFilled(rect.Min, rect.Max, l_ButtonColor,
                                 original_rounding, p_Flags);

        // Draw the text
        draw_list->AddText(
            ImVec2(button_pos.x + 5.0f,
                   button_pos.y + style.FramePadding.y),
            ImGui::GetColorU32(ImGuiCol_Text), p_Text);

        return l_Pressed;
      }

      bool CollapsingHeaderButton(const char *label, u32 p_Flags,
                                  const char *button_label)
      {
        ImGui::PushID(label); // Scope to avoid ID conflicts
        bool is_open = ImGui::CollapsingHeader(label, p_Flags);

        // Calculate header dimensions
        ImVec2 header_min = ImGui::GetItemRectMin();
        ImVec2 header_max = ImGui::GetItemRectMax();
        ImVec2 header_size = ImVec2(header_max.x - header_min.x,
                                    header_max.y - header_min.y);

        // Button size and positioning
        float button_width = ImGui::CalcTextSize(button_label).x +
                             10.0f; // Add padding
        float button_height = ImGui::GetFrameHeight();
        ImVec2 button_pos =
            ImVec2(header_max.x - button_width -
                       ImGui::GetStyle().FramePadding.x,
                   header_min.y);

        // Push clipping rectangle to keep the button inside the
        // header
        ImGui::PushClipRect(header_min, header_max, true);

        // Position and draw the button
        ImGui::SetCursorScreenPos(button_pos);
        if (ImGui::Button(button_label,
                          ImVec2(button_width, button_height))) {
        }

        ImGui::PopClipRect(); // Restore clipping
        ImGui::PopID();

        return is_open;
      }

    } // namespace Gui
  }   // namespace Editor
} // namespace Low
