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

#include "LowRendererImGuiHelper.h"

#include <algorithm>
#include <nfd.h>

#define DRAG_BUTTON_WIDTH 27.0f

namespace Low {
  namespace Editor {
    namespace Gui {

      void Heading2(const char *p_Text)
      {
        ImGui::PushFont(Renderer::ImGuiHelper::fonts().common_500);
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
        using namespace ImGui;

        ImVec2 l_Pos = GetCursorScreenPos();
        // draw_single_letter_label(p_Label, p_Color);

        bool l_Changed = false;

        Util::String l_Label = "##" + p_Label;

        if (drag_float(l_Label.c_str(), p_Value, (p_Width - 16.0f),
                       0.2f, 0.0f, 0.0f)) {
          l_Changed = true;
        }

        {
          SetCursorScreenPos(l_Pos + ImVec2{2.0f, 0.0f});

          ImDrawList *draw_list = ImGui::GetWindowDrawList();

          ImVec2 widget_size = ImVec2(
              4.0f,
              ImGui::GetFrameHeight()); // Customize width as needed

          draw_list->AddRectFilled(
              l_Pos - ImVec2{widget_size.x, 0.0f},
              ImVec2(l_Pos.x, l_Pos.y + widget_size.y), p_Color,
              2.0f,                        // Rounding amount
              ImDrawFlags_RoundCornersLeft // Only round the right
                                           // corners
          );
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

      bool TreeNodeBehavior(ImGuiID id, ImGuiTreeNodeFlags flags,
                            const char *label, const char *label_end)
      {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
          return false;

        const float l_Rounding = 4.0f;

        ImGuiContext &g = *GImGui;
        const ImGuiStyle &style = g.Style;
        const bool display_frame =
            (flags & ImGuiTreeNodeFlags_Framed) != 0;
        const ImVec2 padding =
            (display_frame ||
             (flags & ImGuiTreeNodeFlags_FramePadding))
                ? (style.FramePadding + ImVec2(0.0f, 2.8f))
                : ImVec2(style.FramePadding.x,
                         ImMin(window->DC.CurrLineTextBaseOffset,
                               style.FramePadding.y));

        if (!label_end)
          label_end = ImGui::FindRenderedTextEnd(label);
        const ImVec2 label_size =
            ImGui::CalcTextSize(label, label_end, false);

        // We vertically grow up to current line height up the typical
        // widget height.
        const float frame_height =
            ImMax(ImMin(window->DC.CurrLineSize.y,
                        g.FontSize + style.FramePadding.y * 2),
                  label_size.y + padding.y * 2);
        const bool span_all_columns =
            (flags & ImGuiTreeNodeFlags_SpanAllColumns) != 0 &&
            (g.CurrentTable != NULL);
        ImRect frame_bb;
        frame_bb.Min.x = span_all_columns
                             ? window->ParentWorkRect.Min.x
                         : (flags & ImGuiTreeNodeFlags_SpanFullWidth)
                             ? window->WorkRect.Min.x
                             : window->DC.CursorPos.x;
        frame_bb.Min.y = window->DC.CursorPos.y;
        frame_bb.Max.x = span_all_columns
                             ? window->ParentWorkRect.Max.x
                             : window->WorkRect.Max.x;
        frame_bb.Max.y = window->DC.CursorPos.y + frame_height;
        if (display_frame) {
          // Framed header expand a little outside the default
          // padding, to the edge of InnerClipRect (FIXME: May remove
          // this at some point and make InnerClipRect align with
          // WindowPadding.x instead of WindowPadding.x*0.5f)
          frame_bb.Min.x -=
              IM_TRUNC(window->WindowPadding.x * 0.5f - 1.0f);
          frame_bb.Max.x += IM_TRUNC(window->WindowPadding.x * 0.5f);
        }

        const float text_offset_x =
            g.FontSize +
            (display_frame
                 ? padding.x * 3
                 : padding.x * 2); // Collapsing arrow width + Spacing
        const float text_offset_y = ImMax(
            padding.y,
            window->DC.CurrLineTextBaseOffset); // Latch before
                                                // ItemSize changes it
        const float text_width =
            g.FontSize + (label_size.x > 0.0f
                              ? label_size.x + padding.x * 2
                              : 0.0f); // Include collapsing
        ImVec2 text_pos(window->DC.CursorPos.x + text_offset_x,
                        window->DC.CursorPos.y + text_offset_y);
        ImGui::ItemSize(ImVec2(text_width, frame_height), padding.y);

        // For regular tree nodes, we arbitrary allow to click past 2
        // worth of ItemSpacing
        ImRect interact_bb = frame_bb;
        if (!display_frame &&
            (flags & (ImGuiTreeNodeFlags_SpanAvailWidth |
                      ImGuiTreeNodeFlags_SpanFullWidth |
                      ImGuiTreeNodeFlags_SpanAllColumns)) == 0)
          interact_bb.Max.x = frame_bb.Min.x + text_width +
                              style.ItemSpacing.x * 2.0f;

        // Modify ClipRect for the ItemAdd(), faster than doing a
        // PushColumnsBackground/PushTableBackgroundChannel for every
        // Selectable..
        const float backup_clip_rect_min_x = window->ClipRect.Min.x;
        const float backup_clip_rect_max_x = window->ClipRect.Max.x;
        if (span_all_columns) {
          window->ClipRect.Min.x = window->ParentWorkRect.Min.x;
          window->ClipRect.Max.x = window->ParentWorkRect.Max.x;
        }

        // Compute open and multi-select states before ItemAdd() as it
        // clear NextItem data.
        bool is_open = ImGui::TreeNodeUpdateNextOpen(id, flags);
        bool item_add = ImGui::ItemAdd(interact_bb, id);
        g.LastItemData.StatusFlags |=
            ImGuiItemStatusFlags_HasDisplayRect;
        g.LastItemData.DisplayRect = frame_bb;

        if (span_all_columns) {
          window->ClipRect.Min.x = backup_clip_rect_min_x;
          window->ClipRect.Max.x = backup_clip_rect_max_x;
        }

        // If a NavLeft request is happening and
        // ImGuiTreeNodeFlags_NavLeftJumpsBackHere enabled: Store data
        // for the current depth to allow returning to this node from
        // any child item. For this purpose we essentially compare if
        // g.NavIdIsAlive went from 0 to 1 between TreeNode() and
        // TreePop(). It will become tempting to enable
        // ImGuiTreeNodeFlags_NavLeftJumpsBackHere by default or move
        // it to ImGuiStyle. Currently only supports 32 level deep and
        // we are fine with (1 << Depth) overflowing into a zero, easy
        // to increase.
        if (is_open && !g.NavIdIsAlive &&
            (flags & ImGuiTreeNodeFlags_NavLeftJumpsBackHere) &&
            !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
          if (g.NavMoveDir == ImGuiDir_Left &&
              g.NavWindow == window &&
              ImGui::NavMoveRequestButNoResultYet()) {
            g.NavTreeNodeStack.resize(g.NavTreeNodeStack.Size + 1);
            ImGuiNavTreeNodeData *nav_tree_node_data =
                &g.NavTreeNodeStack.back();
            nav_tree_node_data->ID = id;
            nav_tree_node_data->InFlags = g.LastItemData.InFlags;
            nav_tree_node_data->NavRect = g.LastItemData.NavRect;
            window->DC.TreeJumpToParentOnPopMask |=
                (1 << window->DC.TreeDepth);
          }

        const bool is_leaf = (flags & ImGuiTreeNodeFlags_Leaf) != 0;
        if (!item_add) {
          if (is_open &&
              !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
            ImGui::TreePushOverrideID(id);
          IMGUI_TEST_ENGINE_ITEM_INFO(
              g.LastItemData.ID, label,
              g.LastItemData.StatusFlags |
                  (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) |
                  (is_open ? ImGuiItemStatusFlags_Opened : 0));
          return is_open;
        }

        if (span_all_columns) {
          ImGui::TablePushBackgroundChannel();
          g.LastItemData.StatusFlags |=
              ImGuiItemStatusFlags_HasClipRect;
          g.LastItemData.ClipRect = window->ClipRect;
        }

        ImGuiButtonFlags button_flags = ImGuiTreeNodeFlags_None;
        if ((flags & ImGuiTreeNodeFlags_AllowOverlap) ||
            (g.LastItemData.InFlags & ImGuiItemFlags_AllowOverlap))
          button_flags |= ImGuiButtonFlags_AllowOverlap;
        if (!is_leaf)
          button_flags |= ImGuiButtonFlags_PressedOnDragDropHold;

        // We allow clicking on the arrow section with keyboard
        // modifiers held, in order to easily allow browsing a tree
        // while preserving selection with code implementing
        // multi-selection patterns. When clicking on the rest of the
        // tree node we always disallow keyboard modifiers.
        const float arrow_hit_x1 =
            (text_pos.x - text_offset_x) - style.TouchExtraPadding.x;
        const float arrow_hit_x2 = (text_pos.x - text_offset_x) +
                                   (g.FontSize + padding.x * 2.0f) +
                                   style.TouchExtraPadding.x;
        const bool is_mouse_x_over_arrow =
            (g.IO.MousePos.x >= arrow_hit_x1 &&
             g.IO.MousePos.x < arrow_hit_x2);
        if (window != g.HoveredWindow || !is_mouse_x_over_arrow)
          button_flags |= ImGuiButtonFlags_NoKeyModifiers;

        // Open behaviors can be altered with the _OpenOnArrow and
        // _OnOnDoubleClick flags. Some alteration have subtle effects
        // (e.g. toggle on MouseUp vs MouseDown events) due to
        // requirements for multi-selection and drag and drop support.
        // - Single-click on label = Toggle on MouseUp (default, when
        // _OpenOnArrow=0)
        // - Single-click on arrow = Toggle on MouseDown (when
        // _OpenOnArrow=0)
        // - Single-click on arrow = Toggle on MouseDown (when
        // _OpenOnArrow=1)
        // - Double-click on label = Toggle on MouseDoubleClick (when
        // _OpenOnDoubleClick=1)
        // - Double-click on arrow = Toggle on MouseDoubleClick (when
        // _OpenOnDoubleClick=1 and _OpenOnArrow=0) It is rather
        // standard that arrow click react on Down rather than Up. We
        // set ImGuiButtonFlags_PressedOnClickRelease on
        // OpenOnDoubleClick because we want the item to be active on
        // the initial MouseDown in order for drag and drop to work.
        if (is_mouse_x_over_arrow)
          button_flags |= ImGuiButtonFlags_PressedOnClick;
        else if (flags & ImGuiTreeNodeFlags_OpenOnDoubleClick)
          button_flags |= ImGuiButtonFlags_PressedOnClickRelease |
                          ImGuiButtonFlags_PressedOnDoubleClick;
        else
          button_flags |= ImGuiButtonFlags_PressedOnClickRelease;

        bool selected = (flags & ImGuiTreeNodeFlags_Selected) != 0;
        const bool was_selected = selected;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(
            interact_bb, id, &hovered, &held, button_flags);
        bool toggled = false;
        if (!is_leaf) {
          if (pressed && g.DragDropHoldJustPressedId != id) {
            if ((flags & (ImGuiTreeNodeFlags_OpenOnArrow |
                          ImGuiTreeNodeFlags_OpenOnDoubleClick)) ==
                    0 ||
                (g.NavActivateId == id))
              toggled = true;
            if (flags & ImGuiTreeNodeFlags_OpenOnArrow)
              toggled |=
                  is_mouse_x_over_arrow &&
                  !g.NavDisableMouseHover; // Lightweight equivalent
                                           // of IsMouseHoveringRect()
                                           // since ButtonBehavior()
                                           // already did the job
            if ((flags & ImGuiTreeNodeFlags_OpenOnDoubleClick) &&
                g.IO.MouseClickedCount[0] == 2)
              toggled = true;
          } else if (pressed && g.DragDropHoldJustPressedId == id) {
            IM_ASSERT(button_flags &
                      ImGuiButtonFlags_PressedOnDragDropHold);
            if (!is_open) // When using Drag and Drop "hold to open"
                          // we keep the node highlighted after
                          // opening, but never close it again.
              toggled = true;
          }

          if (g.NavId == id && g.NavMoveDir == ImGuiDir_Left &&
              is_open) {
            toggled = true;
            ImGui::NavClearPreferredPosForAxis(ImGuiAxis_X);
            ImGui::NavMoveRequestCancel();
          }
          if (g.NavId == id && g.NavMoveDir == ImGuiDir_Right &&
              !is_open) // If there's something upcoming on the line
                        // we may want to give it the priority?
          {
            toggled = true;
            ImGui::NavClearPreferredPosForAxis(ImGuiAxis_X);
            ImGui::NavMoveRequestCancel();
          }

          if (toggled) {
            is_open = !is_open;
            window->DC.StateStorage->SetInt(id, is_open);
            g.LastItemData.StatusFlags |=
                ImGuiItemStatusFlags_ToggledOpen;
          }
        }

        // In this branch, TreeNodeBehavior() cannot toggle the
        // selection so this will never trigger.
        if (selected != was_selected) //-V547
          g.LastItemData.StatusFlags |=
              ImGuiItemStatusFlags_ToggledSelection;

        // Render
        const ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
        ImGuiNavHighlightFlags nav_highlight_flags =
            ImGuiNavHighlightFlags_TypeThin;
        if (display_frame) {
          // Framed type
          const ImU32 bg_col = ImGui::GetColorU32(
              (held && hovered) ? ImGuiCol_HeaderActive
              : hovered         ? ImGuiCol_HeaderHovered
                                : ImGuiCol_Header);
          ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, true,
                             l_Rounding);
          ImGui::RenderNavHighlight(frame_bb, id,
                                    nav_highlight_flags);
          if (flags & ImGuiTreeNodeFlags_Bullet) {
            ImGui::RenderBullet(
                window->DrawList,
                ImVec2(text_pos.x - text_offset_x * 0.60f,
                       text_pos.y + g.FontSize * 0.5f),
                text_col);
          } else if (!is_leaf) {
            /*
            ImGui::RenderArrow(
                window->DrawList,
                ImVec2(text_pos.x - text_offset_x + padding.x,
                       text_pos.y),
                text_col,
                is_open
                    ? ((flags & ImGuiTreeNodeFlags_UpsideDownArrow)
                           ? ImGuiDir_Up
                           : ImGuiDir_Down)
                    : ImGuiDir_Right,
                1.0f);
                */
            const char *arrow = is_open ? ICON_LC_CHEVRON_DOWN
                                        : ICON_LC_CHEVRON_RIGHT;
            ImVec2 arrow_pos(text_pos.x - text_offset_x + padding.x,
                             text_pos.y + 1.0f);
            window->DrawList->AddText(arrow_pos, text_col, arrow);
          } else // Leaf without bullet, left-adjusted text
            text_pos.x -= text_offset_x - padding.x;
          if (flags & ImGuiTreeNodeFlags_ClipLabelForTrailingButton)
            frame_bb.Max.x -= g.FontSize + style.FramePadding.x;

          if (g.LogEnabled)
            ImGui::LogSetNextTextDecoration("###", "###");
        } else {
          // Unframed typed for tree nodes
          if (hovered || selected) {
            const ImU32 bg_col = ImGui::GetColorU32(
                (held && hovered) ? ImGuiCol_HeaderActive
                : hovered         ? ImGuiCol_HeaderHovered
                                  : ImGuiCol_Header);
            ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, bg_col,
                               false);
          }
          ImGui::RenderNavHighlight(frame_bb, id,
                                    nav_highlight_flags);
          if (flags & ImGuiTreeNodeFlags_Bullet) {
            ImGui::RenderBullet(
                window->DrawList,
                ImVec2(text_pos.x - text_offset_x * 0.5f,
                       text_pos.y + g.FontSize * 0.5f),
                text_col);
          } else if (!is_leaf) {
            ImGui::RenderArrow(
                window->DrawList,
                ImVec2(text_pos.x - text_offset_x + padding.x,
                       text_pos.y + g.FontSize * 0.15f),
                text_col,
                is_open
                    ? ((flags & ImGuiTreeNodeFlags_UpsideDownArrow)
                           ? ImGuiDir_Up
                           : ImGuiDir_Down)
                    : ImGuiDir_Right,
                0.70f);
          }
          if (g.LogEnabled)
            ImGui::LogSetNextTextDecoration(">", NULL);
        }

        if (span_all_columns)
          ImGui::TablePopBackgroundChannel();

        // Label
        if (display_frame)
          ImGui::RenderTextClipped(text_pos, frame_bb.Max, label,
                                   label_end, &label_size);
        else
          ImGui::RenderText(text_pos, label, label_end, false);

        if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
          ImGui::TreePushOverrideID(id);
        IMGUI_TEST_ENGINE_ITEM_INFO(
            id, label,
            g.LastItemData.StatusFlags |
                (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) |
                (is_open ? ImGuiItemStatusFlags_Opened : 0));

        if (is_open) {
          ImGui::Dummy({0.0f, 4.0f});
        }
        return is_open;
      }

      bool CollapsingHeader(const char *label,
                            ImGuiTreeNodeFlags flags)
      {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
          return false;
        ImGuiID id = window->GetID(label);
        return TreeNodeBehavior(
            id, flags | ImGuiTreeNodeFlags_CollapsingHeader, label);
      }

      bool InputText(Util::String p_Label, char *p_Text, int p_Length,
                     ImGuiInputTextFlags p_Flags)
      {
        Theme &l_Theme = theme_get_current();

        const float rounding = 4.0f;
        const float icon_padding = 8.0f;
        const float spacing = 4.0f;
        const float frame_height = 27.0f;

        float l_FullWidth = ImGui::CalcItemWidth();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 total_size = ImVec2(
            l_FullWidth, frame_height); // Or calc width if dynamic

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

        // Position input field
        ImGui::SameLine(0, spacing);
        ImGui::SetCursorPosY(pos.y); // align with background
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg,
                              IM_COL32(0, 0, 0, 0)); // Transparent
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));

        // Shrink width for input field
        ImGui::PushItemWidth(total_size.x - spacing - 6.0f);

        ImGui::SetCursorScreenPos(ImVec2(
            pos.x + 3.0f,
            pos.y +
                ((frame_height - ImGui::GetTextLineHeight()) * 0.5f) -
                3.0f));

        bool changed = ImGui::InputText(p_Label.c_str(), p_Text,
                                        p_Length, p_Flags);
        ImGui::PopItemWidth();

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);

        ImGui::SetCursorScreenPos({pos.x, pos.y + total_size.y});

        return changed;
      }

    } // namespace Gui
  }   // namespace Editor
} // namespace Low
