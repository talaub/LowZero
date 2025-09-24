#include "LowEditorGui.h"

#include "LowUtilHandle.h"

#include "LowCoreEntity.h"

#include "LowEditorThemes.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontAwesome5.h"
#include "IconsLucide.h"
#include <string.h>
#include "LowEditorIcons.h"

#include <algorithm>
#include <nfd.h>

#define DRAG_BUTTON_WIDTH 27.0f

namespace Low {
  namespace Editor {
    namespace Gui {

      void Heading2(const char *p_Text)
      {
        ImGui::PushFont(Fonts::UI());
        ImGui::TextWrapped(p_Text);
        ImGui::PopFont();
      }

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

        const float l_BorderThickness = 2.0f;

        /*
        draw_list->AddRectFilled(
            cursor_pos - ImVec2{l_BorderThickness, l_BorderThickness},
            ImVec2(cursor_pos.x + widget_size.x,
                   cursor_pos.y + widget_size.y) +
                ImVec2{l_BorderThickness, l_BorderThickness},
            ImGui::GetColorU32(
                color_to_imvec4(theme_get_current().header)),
            2.0f, // Rounding amount
            ImDrawFlags_RoundCornersRight
            // corners
        );
        */
        draw_list->AddRectFilled(cursor_pos,
                                 ImVec2(cursor_pos.x + widget_size.x,
                                        cursor_pos.y + widget_size.y),
                                 ImGui::GetColorU32(bg_color),
                                 2.0f, // Rounding amount
                                 ImDrawFlags_RoundCornersRight
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
        // Width for this field
        ImGui::SetNextItemWidth(p_Width);

        // Invisible label keeps IDs unique without text
        Util::String l_Id = "##" + p_Label;

        // Draw the float input
        bool l_Changed =
            ImGui::DragFloat(l_Id.c_str(), p_Value, 0.2f, 0.0f, 0.0f);

        // Paint the left color strip using the last item's rect.
        // No cursor movement—pure overlay.
        {
          ImDrawList *l_DrawList = ImGui::GetWindowDrawList();
          const ImVec2 l_Min = ImGui::GetItemRectMin();
          const ImVec2 l_Max = ImGui::GetItemRectMax();

          const float l_StripWidth = 4.0f;
          ImVec2 l_StripMin = ImVec2(l_Min.x, l_Min.y);
          ImVec2 l_StripMax = ImVec2(l_Min.x + l_StripWidth, l_Max.y);

          // Round only the left corners so it blends with frame
          // rounding
          l_DrawList->AddRectFilled(l_StripMin, l_StripMax, p_Color,
                                    2.0f,
                                    ImDrawFlags_RoundCornersLeft);
        }

        // If this is not the last field in the row, keep items on the
        // same line
        if (!p_Break) {
          ImGui::SameLine(0.0f, p_Margin);
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
        return draw_single_coefficient_editor(
            p_Label, p_Color, p_Value, p_Width, 0.0f, true);
      }

      bool Vector3Edit(Math::Vector3 &p_Vector, float p_MaxWidth)
      {
        float l_FullWidth = ImGui::GetContentRegionAvail().x;
        if (p_MaxWidth > 0.0f) {
          l_FullWidth = p_MaxWidth;
        }

        const float l_Spacing = LOW_EDITOR_SPACING;
        const float l_Width =
            (l_FullWidth - (l_Spacing * 2.0f)) / 3.0f;

        bool l_Changed = false;
        Theme &l_Theme = theme_get_current();

        ImGui::BeginGroup(); // keep the three fields as one logical
                             // block

        // X
        l_Changed |= draw_single_coefficient_editor(
            "X", color_to_imcolor(l_Theme.coords0), &p_Vector.x,
            l_Width, l_Spacing, false);

        // Y
        l_Changed |= draw_single_coefficient_editor(
            "Y", color_to_imcolor(l_Theme.coords1), &p_Vector.y,
            l_Width, l_Spacing, false);

        // Z (last in the row -> break)
        l_Changed |= draw_single_coefficient_editor(
            "Z", color_to_imcolor(l_Theme.coords2), &p_Vector.z,
            l_Width, /*p_Margin*/ 0.0f, /*p_Break*/ true);

        ImGui::EndGroup();

        // After EndGroup(), ImGui will naturally move to the next
        // line as needed
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
        return false;
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
            Core::Entity l_Entity;
            l_TypeInfo.properties[N(entity)].get(p_Handle, &l_Entity);
            l_Name = l_Entity.get_name();
          } else {
            l_TypeInfo.properties[N(name)].get(p_Handle, &l_Name);
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
        ImGui::PushFont(Fonts::UI());
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

        const float rounding = 15.0f;
        const float icon_padding = 8.0f;
        const float spacing = 4.0f;
        const float frame_height = 27.0f;

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 total_size =
            ImVec2(200.0f, frame_height); // Or calc width if dynamic

        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
          return false;

        ImGui::ItemSize(total_size);
        ImGuiID id = window->GetID(p_Label.c_str());
        ImRect bb(pos,
                  ImVec2(pos.x + total_size.x, pos.y + total_size.y));
        if (!ImGui::ItemAdd(bb, id))
          return false;

        ImU32 bg_col = ImGui::IsItemHovered()
                           ? color_to_imcolor(l_Theme.input)
                           : color_to_imcolor(l_Theme.input);
        ImU32 border_col = color_to_imcolor(l_Theme.button);

        // Background + border
        window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_col,
                                        rounding);
        window->DrawList->AddRect(bb.Min, bb.Max, border_col,
                                  rounding, 0, 1.0f);

        // Position icon
        ImGui::SetCursorScreenPos(ImVec2(
            pos.x + icon_padding,
            pos.y +
                (frame_height - ImGui::GetTextLineHeight()) * 0.5f));
        ImGui::TextUnformatted(LOW_EDITOR_ICON_SEARCH);

        // Position input field
        ImGui::SameLine(0, spacing);
        ImGui::SetCursorPosY(pos.y); // align with background
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg,
                              IM_COL32(0, 0, 0, 0)); // Transparent
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));

        const float l_IconWidth =
            ImGui::CalcTextSize(LOW_EDITOR_ICON_SEARCH).x;

        // Shrink width for input field
        ImGui::PushItemWidth(total_size.x - icon_padding * 2 -
                             l_IconWidth - spacing);

        ImGui::SetCursorScreenPos(
            {pos.x + icon_padding + l_IconWidth, pos.y + 2.0f});
        bool changed = ImGui::InputText(p_Label.c_str(),
                                        p_SearchString, p_Length);
        ImGui::PopItemWidth();

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);

        ImGui::SetCursorScreenPos({pos.x, pos.y + total_size.y});

        return changed;
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
        ImGui::BeginGroup();

        bool l_Edited = false;

        const float l_FullWidth = ImGui::GetContentRegionAvail().x;
        const float l_ButtonWidth =
            DRAG_BUTTON_WIDTH; // your constant
        const float l_Spacing = ImGui::GetStyle().ItemInnerSpacing.x;

        // Drag width accounts for two buttons + two inner spacings
        const float l_DragWidth =
            l_FullWidth - (l_ButtonWidth * 2.0f) - (l_Spacing * 2.0f);

        ImGui::PushID(label);

        // --- Value drag ---
        ImGui::SetNextItemWidth(l_DragWidth);
        if (ImGui::DragFloat("##Drag", value, speed, min, max,
                             format)) {
          if (min < max)
            *value = ImClamp(*value, min, max);
          l_Edited = true;
        }

        // --- Plus button ---
        ImGui::SameLine(0.0f, l_Spacing);
        ImGui::PushItemFlag(
            ImGuiItemFlags_NoNavDefaultFocus,
            true); // keep nav focus on the drag by default
        if (ButtonNoRounding(ICON_LC_PLUS, l_ButtonWidth)) {
          *value += 1.0f;
          if (min < max)
            *value = ImClamp(*value, min, max);
          l_Edited = true;
        }

        // --- Minus button (right-rounded) ---
        ImGui::SameLine(0.0f, l_Spacing);
        if (ButtonRounding(ICON_LC_MINUS,
                           ImDrawFlags_RoundCornersRight,
                           l_ButtonWidth)) {
          *value -= 1.0f;
          if (min < max)
            *value = ImClamp(*value, min, max);
          l_Edited = true;
        }
        ImGui::PopItemFlag();

        ImGui::PopID();
        ImGui::EndGroup();

        return l_Edited;
      }

      // Layout-safe custom buttons that don't move the cursor
      // manually. Optional fixed width ensures consistent layout in
      // rows.

      bool ButtonNoRounding(const char *p_Text,
                            float p_FixedWidth /*=0.0f*/)
      {
        ImGuiStyle &l_Style = ImGui::GetStyle();
        ImDrawList *l_Draw = ImGui::GetWindowDrawList();

        // Compute desired size
        ImVec2 l_TextSize = ImGui::CalcTextSize(p_Text);
        ImVec2 l_Size =
            ImVec2((p_FixedWidth > 0.0f ? p_FixedWidth
                                        : l_TextSize.x + 10.0f),
                   ImGui::GetFrameHeight());

        // Consume layout
        bool l_Pressed = ImGui::InvisibleButton(
            Util::String("##").append(p_Text).c_str(), l_Size);

        // Use the actual item rect to draw (layout-safe)
        ImVec2 l_Min = ImGui::GetItemRectMin();
        ImVec2 l_Max = ImGui::GetItemRectMax();

        ImU32 l_Col = ImGui::GetColorU32(
            ImGui::IsItemActive()    ? ImGuiCol_ButtonActive
            : ImGui::IsItemHovered() ? ImGuiCol_ButtonHovered
                                     : ImGuiCol_Button);

        l_Draw->AddRectFilled(l_Min, l_Max, l_Col, 0.0f);

        // Center text vertically, pad a bit horizontally
        const float l_PadX = 5.0f;
        ImVec2 l_TextPos(l_Min.x + l_PadX,
                         l_Min.y + (l_Size.y - l_TextSize.y) * 0.5f);
        l_Draw->AddText(l_TextPos, ImGui::GetColorU32(ImGuiCol_Text),
                        p_Text);

        return l_Pressed;
      }

      bool ButtonRounding(const char *p_Text, u32 p_Flags,
                          float p_FixedWidth /*=0.0f*/)
      {
        ImGuiStyle &l_Style = ImGui::GetStyle();
        ImDrawList *l_Draw = ImGui::GetWindowDrawList();

        ImVec2 l_TextSize = ImGui::CalcTextSize(p_Text);
        ImVec2 l_Size =
            ImVec2((p_FixedWidth > 0.0f ? p_FixedWidth
                                        : l_TextSize.x + 10.0f),
                   ImGui::GetFrameHeight());

        bool l_Pressed = ImGui::InvisibleButton(
            Util::String("##").append(p_Text).c_str(), l_Size);

        ImVec2 l_Min = ImGui::GetItemRectMin();
        ImVec2 l_Max = ImGui::GetItemRectMax();

        ImU32 l_Col = ImGui::GetColorU32(
            ImGui::IsItemActive()    ? ImGuiCol_ButtonActive
            : ImGui::IsItemHovered() ? ImGuiCol_ButtonHovered
                                     : ImGuiCol_Button);

        l_Draw->AddRectFilled(l_Min, l_Max, l_Col,
                              l_Style.FrameRounding, p_Flags);

        const float l_PadX = 5.0f;
        ImVec2 l_TextPos(l_Min.x + l_PadX,
                         l_Min.y + (l_Size.y - l_TextSize.y) * 0.5f);
        l_Draw->AddText(l_TextPos, ImGui::GetColorU32(ImGuiCol_Text),
                        p_Text);

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

      bool Button(const char *p_Label, bool p_Disabled,
                  const char *p_Icon, Low::Math::Color p_IconColor)
      {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
          return false;

        ImGuiContext &g = *ImGui::GetCurrentContext();
        ImGuiStyle &style = g.Style;

        ImVec2 l_LabelSize = ImGui::CalcTextSize(p_Label);
        ImVec2 l_IconSize{15.0f, 15.0f};
        const float l_TextIconSpacing = 7.0f;
        const ImVec2 l_Spacing{10.0f, 5.0f};
        const float l_ContentHeight =
            LOW_MATH_MAX(l_IconSize.y, l_LabelSize.y);

        ImVec2 l_Size =
            ImVec2(l_IconSize.x + l_LabelSize.x + l_TextIconSpacing +
                       (l_Spacing.x * 2.0f),
                   l_ContentHeight + (l_Spacing.y * 2.0f));
        if (p_Icon == nullptr) {
          l_Size = ImVec2(l_LabelSize.x + (l_Spacing.x * 2.0f),
                          l_ContentHeight + (l_Spacing.y * 2.0f));
        }

        ImVec2 l_Position = ImGui::GetCursorScreenPos();
        ImRect bb(l_Position, ImVec2(l_Position.x + l_Size.x,
                                     l_Position.y + l_Size.y));
        ImGui::ItemSize(bb);
        ImGuiID id = window->GetID(p_Label);
        if (!ImGui::ItemAdd(bb, id))
          return false;

        bool hovered = ImGui::IsItemHovered();
        bool held = ImGui::IsItemActive();
        ImU32 border_col =
            color_to_imcolor(theme_get_current().buttonBorder);
        ImU32 bg_col =
            held ? color_to_imcolor(theme_get_current().buttonActive)
            : hovered
                ? color_to_imcolor(theme_get_current().buttonHover)
                : color_to_imcolor(theme_get_current().button);

        float rounding = 3.0f;

        // Draw background and border
        window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_col,
                                        rounding);
        window->DrawList->AddRect(bb.Min, bb.Max, border_col,
                                  rounding, 0, 1.0f);

        if (p_Icon != nullptr) {
          const ImVec2 l_IconPosition{l_Position.x + l_Spacing.x,
                                      l_Position.y + l_Spacing.y};
          ImGui::PushStyleColor(ImGuiCol_Text,
                                color_to_imvec4(Math::Color(
                                    p_IconColor.r, p_IconColor.g,
                                    p_IconColor.b, 1.0f)));
          ImGui::RenderText(l_IconPosition, p_Icon);
          ImGui::PopStyleColor();
        }

        // Draw label text
        ImVec2 l_TextPos{l_Position.x + l_Spacing.x + l_IconSize.x +
                             l_TextIconSpacing,
                         l_Position.y + l_Spacing.y};

        if (p_Icon == nullptr) {
          l_TextPos.x = l_Position.x + l_Spacing.x;
        }

        ImGui::PushClipRect(bb.Min, bb.Max, true);
        ImGui::RenderText(l_TextPos, p_Label);
        ImGui::PopClipRect();

        ImGui::SetCursorScreenPos(l_Position);
        return ImGui::InvisibleButton(p_Label, l_Size);
      }

      bool AddButton(bool p_Disabled)
      {
        return Button("Add", p_Disabled, LOW_EDITOR_ICON_ADD,
                      theme_get_current().add);
      }

      bool AddButton(const char *p_Label, bool p_Disabled)
      {
        return Button(p_Label, p_Disabled, LOW_EDITOR_ICON_ADD,
                      theme_get_current().add);
      }

      bool EditButton(bool p_Disabled)
      {
        return Button("Edit", p_Disabled, ICON_LC_PENCIL,
                      theme_get_current().edit);
      }

      bool SaveButton(bool p_Disabled)
      {
        return Button("Save", p_Disabled, LOW_EDITOR_ICON_SAVE,
                      theme_get_current().save);
      }

      bool ClearButton(bool p_Disabled)
      {
        return Button("Clear", p_Disabled, LOW_EDITOR_ICON_CLEAR,
                      theme_get_current().clear);
      }

      bool DeleteButton(bool p_Disabled)
      {
        return Button("Delete", p_Disabled, LOW_EDITOR_ICON_DELETE,
                      theme_get_current().remove);
      }

      bool ToggleButtonSimple(const char *p_Label, bool *p_Value)
      {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
          return false;

        ImGuiStyle &style = ImGui::GetStyle();
        ImGuiContext &g = *ImGui::GetCurrentContext();

        const float rounding = 4.0f;
        const float circle_radius = 6.0f;
        const float spacing = 8.0f;

        const char *label = *p_Value ? "ON" : "OFF";
        ImVec2 text_size = ImGui::CalcTextSize("OFF");

        ImVec2 l_IconSize{15.0f, 15.0f};
        const float l_TextIconSpacing = 7.0f;
        const ImVec2 l_Spacing{10.0f, 5.0f};

        float width = l_Spacing.x * 2 + circle_radius * 2 +
                      l_TextIconSpacing + text_size.x;
        float height = text_size.y + (l_Spacing.y * 2);

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 size = ImVec2(width, height);
        ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb);
        ImGuiID button_id = window->GetID(p_Label);

        if (!ImGui::ItemAdd(bb, button_id))
          return false;

        bool hovered, held;
        bool pressed =
            ImGui::ButtonBehavior(bb, button_id, &hovered, &held);

        if (pressed)
          *p_Value = !*p_Value;

        // Colors
        ImU32 border_col =
            color_to_imcolor(theme_get_current().buttonBorder);
        ImU32 bg_col =
            held ? color_to_imcolor(theme_get_current().buttonActive)
            : hovered
                ? color_to_imcolor(theme_get_current().buttonHover)
                : color_to_imcolor(theme_get_current().button);
        ImU32 circle_col =
            *p_Value ? color_to_imcolor(theme_get_current().success)
                     : color_to_imcolor(theme_get_current().error);

        // Draw background + border
        window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_col,
                                        rounding);
        window->DrawList->AddRect(bb.Min, bb.Max, border_col,
                                  rounding, 0, 1.0f);

        // Draw colored circle
        ImVec2 circle_center =
            ImVec2(pos.x + l_Spacing.x + circle_radius,
                   1.0f + pos.y + height * 0.5f);
        window->DrawList->AddCircleFilled(
            circle_center, circle_radius, circle_col, 16);

        // Draw ON/OFF text
        ImVec2 text_pos =
            ImVec2(circle_center.x + circle_radius + spacing,
                   pos.y + (height - text_size.y) * 0.5f);
        window->DrawList->AddText(
            text_pos, color_to_imcolor(theme_get_current().text),
            label);

        return pressed;
      }

      bool CollapsingHeader(const char *label,
                            ImGuiTreeNodeFlags flags)
      {
        // ImGuiWindow *window = ImGui::GetCurrentWindow();
        // if (window->SkipItems)
        //   return false;
        // ImGuiID id = window->GetID(label);
        // return TreeNodeBehavior(
        //     id, flags | ImGuiTreeNodeFlags_CollapsingHeader,
        //     label);
        return ImGui::CollapsingHeader(label, flags);
      }

      bool InputText(Util::String p_Label, char *p_Text, int p_Length,
                     ImGuiInputTextFlags p_Flags)
      {
        Theme &l_Theme = theme_get_current();

        // Target visual height similar to your original (27px).
        // We adapt padding so the final frame height approximates
        // this.
        const float l_TargetFrameHeight = 27.0f;
        const float l_Rounding = 4.0f;
        const float l_BorderSize = 1.0f;

        ImGui::BeginGroup();

        // Width honors current layout context (columns, tables, etc.)
        const float l_FullWidth = ImGui::CalcItemWidth();
        ImGui::SetNextItemWidth(l_FullWidth);

        // Compute Y padding so the widget reaches ~target height.
        // frame_height ~= text_line_height + 2*pad_y
        const float l_TextLineHeight = ImGui::GetTextLineHeight();
        float l_PadY = 0.0f;
        if (l_TargetFrameHeight > l_TextLineHeight) {
          l_PadY = (l_TargetFrameHeight - l_TextLineHeight) * 0.5f;
        }

        // Style: rounding, border, background + hovered/active
        // variants, border color.
        ImGuiStyle &l_Style = ImGui::GetStyle();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                            ImVec2(l_Style.FramePadding.x, l_PadY));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, l_Rounding);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,
                            l_BorderSize);

        // Use your theme colors; reuse input for all three bg states
        // for consistency.
        const ImU32 l_Bg = color_to_imcolor(l_Theme.input);
        const ImU32 l_BgHover = color_to_imcolor(l_Theme.input);
        const ImU32 l_BgActive = color_to_imcolor(l_Theme.input);
        const ImU32 l_Border = color_to_imcolor(l_Theme.button);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, l_Bg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, l_BgHover);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, l_BgActive);
        ImGui::PushStyleColor(ImGuiCol_Border, l_Border);

        // Invisible label -> stable ID without visible text.
        Util::String l_Id = "##";
        l_Id += p_Label;

        const bool l_Changed = ImGui::InputText(
            l_Id.c_str(), p_Text, (size_t)p_Length, p_Flags);

        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(3);

        ImGui::EndGroup();
        return l_Changed;
      }

    } // namespace Gui
  } // namespace Editor
} // namespace Low
