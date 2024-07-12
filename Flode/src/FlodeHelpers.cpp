#define IMGUI_DEFINE_MATH_OPERATORS

#include "FlodeHelpers.h"

#include "imgui_node_editor.h"
#include "imgui_node_editor_internal.h"

namespace ed = ax::NodeEditor;

namespace Flode {
  namespace Helper {
    using namespace ImGui;
    bool BeginNodeCombo(const char *label, const char *preview_value,
                        ImGuiComboFlags flags)
    {
      ImGuiContext &g = *GImGui;
      ImGuiWindow *window = GetCurrentWindow();

      ImGuiNextWindowDataFlags backup_next_window_data_flags =
          g.NextWindowData.Flags;
      g.NextWindowData.ClearFlags(); // We behave like Begin() and
                                     // need to consume those values
      if (window->SkipItems)
        return false;

      const ImGuiStyle &style = g.Style;
      const ImGuiID id = window->GetID(label);
      IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton |
                          ImGuiComboFlags_NoPreview)) !=
                (ImGuiComboFlags_NoArrowButton |
                 ImGuiComboFlags_NoPreview)); // Can't use both flags
                                              // together
      if (flags & ImGuiComboFlags_WidthFitPreview)
        IM_ASSERT(
            (flags &
             (ImGuiComboFlags_NoPreview |
              (ImGuiComboFlags)ImGuiComboFlags_CustomPreview)) == 0);

      const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton)
                                   ? 0.0f
                                   : GetFrameHeight();
      const ImVec2 label_size = CalcTextSize(label, NULL, true);
      const float preview_width =
          ((flags & ImGuiComboFlags_WidthFitPreview) &&
           (preview_value != NULL))
              ? CalcTextSize(preview_value, NULL, true).x
              : 0.0f;
      const float w = (flags & ImGuiComboFlags_NoPreview)
                          ? arrow_size
                          : ((flags & ImGuiComboFlags_WidthFitPreview)
                                 ? (arrow_size + preview_width +
                                    style.FramePadding.x * 2.0f)
                                 : CalcItemWidth());
      const ImRect bb(
          window->DC.CursorPos,
          window->DC.CursorPos +
              ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
      const ImRect total_bb(
          bb.Min, bb.Max + ImVec2(label_size.x > 0.0f
                                      ? style.ItemInnerSpacing.x +
                                            label_size.x
                                      : 0.0f,
                                  0.0f));
      ItemSize(total_bb, style.FramePadding.y);
      if (!ItemAdd(total_bb, id, &bb))
        return false;

      // Open on click
      bool hovered, held;
      bool pressed = ButtonBehavior(bb, id, &hovered, &held);
      const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);
      bool popup_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
      if (pressed && !popup_open) {
        OpenPopupEx(popup_id, ImGuiPopupFlags_None);
        popup_open = true;
      }

      // Render shape
      const ImU32 frame_col = GetColorU32(
          hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
      const float value_x2 = ImMax(bb.Min.x, bb.Max.x - arrow_size);
      RenderNavHighlight(bb, id);
      if (!(flags & ImGuiComboFlags_NoPreview))
        window->DrawList->AddRectFilled(
            bb.Min, ImVec2(value_x2, bb.Max.y), frame_col,
            style.FrameRounding,
            (flags & ImGuiComboFlags_NoArrowButton)
                ? ImDrawFlags_RoundCornersAll
                : ImDrawFlags_RoundCornersLeft);
      if (!(flags & ImGuiComboFlags_NoArrowButton)) {
        ImU32 bg_col = GetColorU32((popup_open || hovered)
                                       ? ImGuiCol_ButtonHovered
                                       : ImGuiCol_Button);
        ImU32 text_col = GetColorU32(ImGuiCol_Text);
        window->DrawList->AddRectFilled(
            ImVec2(value_x2, bb.Min.y), bb.Max, bg_col,
            style.FrameRounding,
            (w <= arrow_size) ? ImDrawFlags_RoundCornersAll
                              : ImDrawFlags_RoundCornersRight);
        if (value_x2 + arrow_size - style.FramePadding.x <= bb.Max.x)
          RenderArrow(window->DrawList,
                      ImVec2(value_x2 + style.FramePadding.y,
                             bb.Min.y + style.FramePadding.y),
                      text_col, ImGuiDir_Down, 1.0f);
      }
      RenderFrameBorder(bb.Min, bb.Max, style.FrameRounding);

      // Custom preview
      if (flags & ImGuiComboFlags_CustomPreview) {
        g.ComboPreviewData.PreviewRect =
            ImRect(bb.Min.x, bb.Min.y, value_x2, bb.Max.y);
        IM_ASSERT(preview_value == NULL || preview_value[0] == 0);
        preview_value = NULL;
      }

      // Render preview and label
      if (preview_value != NULL &&
          !(flags & ImGuiComboFlags_NoPreview)) {
        if (g.LogEnabled)
          LogSetNextTextDecoration("{", "}");
        RenderTextClipped(bb.Min + style.FramePadding,
                          ImVec2(value_x2, bb.Max.y), preview_value,
                          NULL, NULL);
      }
      if (label_size.x > 0)
        RenderText(ImVec2(bb.Max.x + style.ItemInnerSpacing.x,
                          bb.Min.y + style.FramePadding.y),
                   label);

      if (!popup_open)
        return false;

      g.NextWindowData.Flags = backup_next_window_data_flags;
      ed::Suspend();
      return BeginComboPopup(popup_id, bb, flags);
    }

    void EndNodeCombo()
    {
      EndPopup();
      ed::Resume();
    }
  } // namespace Helper
} // namespace Flode
