#include "LowEditorViewportUi.h"

#include "LowCore.h"
#include "LowCoreTweenSystem.h"
#include "LowEditorThemes.h"
#include "LowMath.h"

#include "IconsLucide.h"
#include "imgui_internal.h"

namespace Low {
  namespace Editor {
    namespace ViewportUi {
      static float resolve_scale(float p_Scale)
      {
        if (p_Scale <= 0.0f) {
          return ImGui::GetIO().FontGlobalScale;
        }
        return p_Scale;
      }

      static ImVec2 lerp(const ImVec2 &p_A, const ImVec2 &p_B,
                         const ImVec2 &p_T)
      {
        return ImVec2(p_A.x + (p_B.x - p_A.x) * p_T.x,
                      p_A.y + (p_B.y - p_A.y) * p_T.y);
      }

      void draw_frosted_rect(ImDrawList *p_Draw,
                             ImTextureID p_Background,
                             const ImVec2 &p_RectMin,
                             const ImVec2 &p_RectMax,
                             const ImVec2 &p_SceneRectMin,
                             const ImVec2 &p_SceneRectMax,
                             const FrostedRectStyle &p_Style)
      {
        if (!p_Draw) {
          return;
        }

        ImVec2 l_OverlapMin(ImMax(p_RectMin.x, p_SceneRectMin.x),
                            ImMax(p_RectMin.y, p_SceneRectMin.y));
        ImVec2 l_OverlapMax(ImMin(p_RectMax.x, p_SceneRectMax.x),
                            ImMin(p_RectMax.y, p_SceneRectMax.y));

        if (p_Background && l_OverlapMax.x > l_OverlapMin.x &&
            l_OverlapMax.y > l_OverlapMin.y) {
          ImVec2 l_SceneSize(
              p_SceneRectMax.x - p_SceneRectMin.x,
              p_SceneRectMax.y - p_SceneRectMin.y);
          if (l_SceneSize.x > 0.0f && l_SceneSize.y > 0.0f) {
            ImVec2 l_NormMin(
                (l_OverlapMin.x - p_SceneRectMin.x) /
                    l_SceneSize.x,
                (l_OverlapMin.y - p_SceneRectMin.y) /
                    l_SceneSize.y);
            ImVec2 l_NormMax(
                (l_OverlapMax.x - p_SceneRectMin.x) /
                    l_SceneSize.x,
                (l_OverlapMax.y - p_SceneRectMin.y) /
                    l_SceneSize.y);

            ImVec2 l_UVMin =
                lerp(ImVec2(0, 0), ImVec2(1, 1), l_NormMin);
            ImVec2 l_UVMax =
                lerp(ImVec2(0, 0), ImVec2(1, 1), l_NormMax);

#if IMGUI_VERSION_NUM >= 19080
            p_Draw->AddImageRounded(
                p_Background, l_OverlapMin, l_OverlapMax, l_UVMin,
                l_UVMax, IM_COL32_WHITE, p_Style.radius,
                ImDrawFlags_RoundCornersAll);
#else
            p_Draw->AddImage(p_Background, l_OverlapMin,
                             l_OverlapMax, l_UVMin, l_UVMax,
                             IM_COL32_WHITE);
#endif
          }
        } else if (!p_Background) {
          p_Draw->AddRectFilled(
              p_RectMin, p_RectMax,
              ImGui::GetColorU32(ImGuiCol_WindowBg), p_Style.radius,
              ImDrawFlags_RoundCornersAll);
        }

        p_Draw->AddRectFilled(p_RectMin, p_RectMax, p_Style.tint,
                              p_Style.radius,
                              ImDrawFlags_RoundCornersAll);

        if (p_Style.border_thickness > 0.0f &&
            (p_Style.border & IM_COL32_A_MASK) != 0u) {
          p_Draw->AddRect(p_RectMin, p_RectMax, p_Style.border,
                          p_Style.radius, ImDrawFlags_RoundCornersAll,
                          p_Style.border_thickness);
        }
      }

      bool render_frosted_icon_button_at(
          const char *p_Id, const char *p_Icon,
          const char *p_Tooltip, ImTextureID p_Background,
          const ImVec2 &p_SceneRectMin,
          const ImVec2 &p_SceneRectMax,
          const ImVec2 &p_ButtonCenter,
          const IconButtonStyle &p_Style)
      {
        const float l_Scale = resolve_scale(p_Style.dpi_scale);
        const float l_ButtonSize = p_Style.button_size * l_Scale;
        const float l_Radius = p_Style.corner_radius * l_Scale;
        const float l_IconSize = p_Style.icon_size * l_Scale;

        const ImVec2 l_ButtonMin(
            p_ButtonCenter.x - l_ButtonSize * 0.5f,
            p_ButtonCenter.y - l_ButtonSize * 0.5f);
        const ImVec2 l_ButtonMax(
            p_ButtonCenter.x + l_ButtonSize * 0.5f,
            p_ButtonCenter.y + l_ButtonSize * 0.5f);

        FrostedRectStyle l_FrostedStyle;
        l_FrostedStyle.radius = l_Radius;
        draw_frosted_rect(ImGui::GetWindowDrawList(), p_Background,
                          l_ButtonMin, l_ButtonMax, p_SceneRectMin,
                          p_SceneRectMax, l_FrostedStyle);

        ImGui::PushID(p_Id);
        ImGui::SetCursorScreenPos(l_ButtonMin);
        ImGui::InvisibleButton("button",
                               ImVec2(l_ButtonSize, l_ButtonSize));

        ImDrawList *l_Draw = ImGui::GetWindowDrawList();
        const bool l_Hovered =
            ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly);
        const bool l_Held = ImGui::IsItemActive();
        const bool l_Pressed =
            ImGui::IsItemClicked(ImGuiMouseButton_Left);

        if (l_Hovered || l_Held) {
          const ImU32 l_Fill = ImGui::GetColorU32(
              l_Held ? ImGuiCol_ButtonActive
                     : ImGuiCol_ButtonHovered);
          l_Draw->AddRectFilled(l_ButtonMin, l_ButtonMax, l_Fill,
                                l_Radius * 0.6f,
                                ImDrawFlags_RoundCornersAll);
        }

        if (p_Icon) {
          ImVec2 l_TextSize = ImGui::CalcTextSize(p_Icon);
          const float l_FontSize = ImGui::GetFontSize();
          const float l_TextScale =
              (l_TextSize.y > 0.0f)
                  ? (l_IconSize / l_TextSize.y)
                  : 1.0f;
          ImVec2 l_SizeScaled(l_TextSize.x * l_TextScale,
                              l_TextSize.y * l_TextScale);
          ImVec2 l_TextPos(
              p_ButtonCenter.x - l_SizeScaled.x * 0.5f,
              p_ButtonCenter.y - l_SizeScaled.y * 0.5f);
          l_Draw->AddText(ImGui::GetFont(),
                          l_FontSize * l_TextScale, l_TextPos,
                          ImGui::GetColorU32(ImGuiCol_Text),
                          p_Icon);
        }

        if (l_Hovered && p_Tooltip && *p_Tooltip) {
          ImGui::BeginTooltip();
          ImGui::TextUnformatted(p_Tooltip);
          ImGui::EndTooltip();
        }

        ImGui::PopID();
        return l_Pressed;
      }

      bool render_playmode_button_at(
          ImTextureID p_Background, const ImVec2 &p_SceneRectMin,
          const ImVec2 &p_SceneRectMax,
          const ImVec2 &p_ButtonCenter,
          const IconButtonStyle &p_Style)
      {
        const Util::EngineState l_EngineState =
            Core::get_engine_state();

        if (l_EngineState == Util::EngineState::EDITING) {
          return render_frosted_icon_button_at(
              "PlayModeButton", ICON_LC_PLAY, "Play", p_Background,
              p_SceneRectMin, p_SceneRectMax, p_ButtonCenter,
              p_Style);
        }

        if (l_EngineState == Util::EngineState::PLAYING) {
          return render_frosted_icon_button_at(
              "PlayModeButton", ICON_LC_PAUSE, "Stop", p_Background,
              p_SceneRectMin, p_SceneRectMax, p_ButtonCenter,
              p_Style);
        }

        return false;
      }

      u32 render_horizontal_button_toolbar_at(
          ImTextureID p_Background, const ImVec2 &p_SceneRectMin,
          const ImVec2 &p_SceneRectMax, const ImVec2 &p_Position,
          const Util::List<ToolbarItem> &p_Items,
          const ButtonToolbarStyle &p_StyleIn)
      {
        if (p_Items.empty()) {
          return 0u;
        }

        ButtonToolbarStyle l_Style = p_StyleIn;
        const float l_Scale = resolve_scale(l_Style.dpi_scale);
        const float l_PadX = l_Style.pad_x * l_Scale;
        const float l_PadY = l_Style.pad_y * l_Scale;
        const float l_Gap = l_Style.item_gap * l_Scale;
        const float l_Radius = l_Style.corner_radius * l_Scale;
        const float l_Icon = l_Style.icon_size * l_Scale;

        const float l_BarW =
            l_PadX * 2.0f + l_Icon * p_Items.size() +
            l_Gap * (float)(p_Items.size() - 1);
        const float l_BarH = l_PadY * 2.0f + l_Icon;
        const ImVec2 l_BarMin = p_Position;
        const ImVec2 l_BarMax(p_Position.x + l_BarW,
                              p_Position.y + l_BarH);

        FrostedRectStyle l_FrostedStyle;
        l_FrostedStyle.radius = l_Radius;
        draw_frosted_rect(ImGui::GetWindowDrawList(), p_Background,
                          l_BarMin, l_BarMax, p_SceneRectMin,
                          p_SceneRectMax, l_FrostedStyle);

        ImDrawList *l_Draw = ImGui::GetWindowDrawList();
        ImVec2 l_Cursor(l_BarMin.x + l_PadX, l_BarMin.y + l_PadY);
        const ImVec2 l_ItemSize(l_Icon, l_Icon);
        u32 l_ClickedItem = 0u;

        ImGui::PushID("ViewportUiButtonToolbar");

        for (const ToolbarItem &i_Item : p_Items) {
          ImGui::PushID((int)i_Item.id);
          ImGui::SetCursorScreenPos(l_Cursor);
          ImGui::InvisibleButton("btn", l_ItemSize);

          const bool l_Hovered =
              ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly);
          const bool l_Held = ImGui::IsItemActive();

          if (l_Hovered || l_Held) {
            const ImU32 l_Fill = ImGui::GetColorU32(
                l_Held ? ImGuiCol_ButtonActive
                       : ImGuiCol_ButtonHovered);
            l_Draw->AddRectFilled(ImGui::GetItemRectMin(),
                                  ImGui::GetItemRectMax(), l_Fill,
                                  l_Radius * 0.6f,
                                  ImDrawFlags_RoundCornersAll);
          }

          const ImVec2 l_Center(l_Cursor.x + l_ItemSize.x * 0.5f,
                                l_Cursor.y + l_ItemSize.y * 0.5f);
          if (i_Item.icon_texture) {
            l_Draw->AddImage(
                i_Item.icon_texture,
                ImVec2(l_Center.x - l_Icon * 0.5f,
                       l_Center.y - l_Icon * 0.5f),
                ImVec2(l_Center.x + l_Icon * 0.5f,
                       l_Center.y + l_Icon * 0.5f),
                ImVec2(0, 0), ImVec2(1, 1),
                ImGui::GetColorU32(ImGuiCol_Text));
          } else if (i_Item.icon_glyph) {
            ImVec2 l_TextSize =
                ImGui::CalcTextSize(i_Item.icon_glyph);
            const float l_FontSize = ImGui::GetFontSize();
            const float l_TextScale =
                (l_TextSize.y > 0.0f)
                    ? (l_Icon / l_TextSize.y)
                    : 1.0f;
            const ImVec2 l_SizeScaled(l_TextSize.x * l_TextScale,
                                      l_TextSize.y * l_TextScale);
            l_Draw->AddText(
                ImGui::GetFont(), l_FontSize * l_TextScale,
                ImVec2(l_Center.x - l_SizeScaled.x * 0.5f,
                       l_Center.y - l_SizeScaled.y * 0.5f),
                ImGui::GetColorU32(ImGuiCol_Text),
                i_Item.icon_glyph);
          }

          if (l_Hovered && i_Item.label && *i_Item.label) {
            ImGui::BeginTooltip();
            if (i_Item.shortcut_label && *i_Item.shortcut_label) {
              ImGui::Text("%s  (%s)", i_Item.label,
                          i_Item.shortcut_label);
            } else {
              ImGui::TextUnformatted(i_Item.label);
            }
            ImGui::EndTooltip();
          }

          if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            l_ClickedItem = i_Item.id;
          }

          l_Cursor.x += l_ItemSize.x + l_Gap;
          ImGui::PopID();
        }

        ImGui::PopID();
        return l_ClickedItem;
      }

      u32 render_standard_bottom_button_toolbar(
          ImTextureID p_Background,
          const Util::List<ToolbarItem> &p_Items,
          const ButtonToolbarStyle &p_StyleIn)
      {
        if (p_Items.empty()) {
          return 0u;
        }

        ButtonToolbarStyle l_Style = p_StyleIn;
        const float l_Scale = resolve_scale(l_Style.dpi_scale);
        const float l_Margin = l_Style.margin * l_Scale;
        const float l_PadX = l_Style.pad_x * l_Scale;
        const float l_PadY = l_Style.pad_y * l_Scale;
        const float l_Gap = l_Style.item_gap * l_Scale;
        const float l_Icon = l_Style.icon_size * l_Scale;

        const ImVec2 l_SceneRectMin = ImGui::GetItemRectMin();
        const ImVec2 l_SceneRectMax = ImGui::GetItemRectMax();

        ImGuiWindow *l_Window = ImGui::GetCurrentWindow();
        if (!l_Window) {
          return 0u;
        }

        const ImVec2 l_WinPos = l_Window->Pos;
        const ImVec2 l_ContentMin(
            l_WinPos.x + ImGui::GetWindowContentRegionMin().x,
            l_WinPos.y + ImGui::GetWindowContentRegionMin().y);
        const ImVec2 l_ContentMax(
            l_WinPos.x + ImGui::GetWindowContentRegionMax().x,
            l_WinPos.y + ImGui::GetWindowContentRegionMax().y);

        const float l_BarW =
            l_PadX * 2.0f + l_Icon * p_Items.size() +
            l_Gap * (float)(p_Items.size() - 1);
        const float l_BarH = l_PadY * 2.0f + l_Icon;

        ImVec2 l_Position;
        l_Position.x =
            l_ContentMin.x +
            ((l_ContentMax.x - l_ContentMin.x) - l_BarW) * 0.5f;
        l_Position.y = l_ContentMax.y - l_Margin - l_BarH +
                       l_Style.offset_y * l_Scale;

        return render_horizontal_button_toolbar_at(
            p_Background, l_SceneRectMin, l_SceneRectMax, l_Position,
            p_Items, l_Style);
      }

      void render_vertical_toolbar_at(
          ImTextureID p_Background, const ImVec2 &p_SceneRectMin,
          const ImVec2 &p_SceneRectMax, const ImVec2 &p_Position,
          const Util::List<ToolbarItem> &p_Items,
          ToolbarState &p_State, const ToolbarStyle &p_StyleIn)
      {
        if (p_Items.empty()) {
          return;
        }

        ToolbarStyle l_Style = p_StyleIn;
        const float l_Scale = resolve_scale(l_Style.dpi_scale);
        const float l_PadX = l_Style.pad_x * l_Scale;
        const float l_PadY = l_Style.pad_y * l_Scale;
        const float l_Gap = l_Style.item_gap * l_Scale;
        const float l_Radius = l_Style.corner_radius * l_Scale;
        const float l_Icon = l_Style.icon_size * l_Scale;

        const float l_BarW = l_PadX * 2.0f + l_Icon;
        const float l_BarH =
            l_PadY * 2.0f + l_Icon * p_Items.size() +
            l_Gap * (float)(p_Items.size() - 1);
        const ImVec2 l_BarMin = p_Position;
        const ImVec2 l_BarMax(p_Position.x + l_BarW,
                              p_Position.y + l_BarH);

        FrostedRectStyle l_FrostedStyle;
        l_FrostedStyle.radius = l_Radius;
        draw_frosted_rect(ImGui::GetWindowDrawList(), p_Background,
                          l_BarMin, l_BarMax, p_SceneRectMin,
                          p_SceneRectMax, l_FrostedStyle);

        ImGui::PushClipRect(l_BarMin, l_BarMax, true);

        float l_TargetMarkerY = l_BarMin.y + l_PadY + l_Icon * 0.5f;
        for (u32 i = 0u; i < p_Items.size(); ++i) {
          if (p_Items[i].id == p_State.active_item) {
            l_TargetMarkerY = l_BarMin.y + l_PadY +
                              (l_Icon + l_Gap) * (float)i +
                              l_Icon * 0.5f;
            break;
          }
        }

        if (!p_State.marker_initialized) {
          p_State.marker_initialized = true;
          p_State.previous_active_item = p_State.active_item;
          p_State.marker_start_y = l_TargetMarkerY;
          p_State.marker_target_y = l_TargetMarkerY;
          p_State.marker_y = l_TargetMarkerY;
        }

        if (p_State.active_item != p_State.previous_active_item) {
          if (p_State.marker_tween.is_alive()) {
            p_State.marker_tween.destroy();
          }
          p_State.marker_start_y = p_State.marker_y;
          p_State.marker_target_y = l_TargetMarkerY;
          p_State.marker_tween =
              Core::Tween::start(0.26f, Core::TweenEase::OUTCUBIC);
          p_State.previous_active_item = p_State.active_item;
        } else if (!p_State.marker_tween.is_alive()) {
          p_State.marker_y = l_TargetMarkerY;
          p_State.marker_start_y = l_TargetMarkerY;
          p_State.marker_target_y = l_TargetMarkerY;
        }

        if (p_State.marker_tween.is_alive()) {
          const float l_MarkerProgress =
              Core::System::Tween::get_eased_progress(
                  p_State.marker_tween);
          p_State.marker_y = Math::Util::lerp(
              p_State.marker_start_y, p_State.marker_target_y,
              l_MarkerProgress);
          if (p_State.marker_tween.is_finished()) {
            p_State.marker_tween.destroy();
          }
        }

        ImDrawList *l_Draw = ImGui::GetWindowDrawList();
        {
          const ImVec4 l_DebugColor =
              color_to_imvec4(theme_get_current().debug);
          ImVec4 l_GlowColor = l_DebugColor;
          l_GlowColor.w *= 0.22f;
          const float l_MarkerW = 2.0f * l_Scale;
          const float l_GlowW = 5.0f * l_Scale;
          const float l_MarkerH = l_Icon * 1.16f;
          const float l_MarkerX = l_BarMin.x + 3.0f * l_Scale;
          l_Draw->AddRectFilled(
              ImVec2(l_MarkerX - 1.5f * l_Scale,
                     p_State.marker_y - l_MarkerH * 0.5f),
              ImVec2(l_MarkerX - 1.5f * l_Scale + l_GlowW,
                     p_State.marker_y + l_MarkerH * 0.5f),
              ImGui::GetColorU32(l_GlowColor), l_GlowW * 0.5f,
              ImDrawFlags_RoundCornersAll);
          l_Draw->AddRectFilled(
              ImVec2(l_MarkerX,
                     p_State.marker_y - l_MarkerH * 0.5f),
              ImVec2(l_MarkerX + l_MarkerW,
                     p_State.marker_y + l_MarkerH * 0.5f),
              ImGui::GetColorU32(l_DebugColor), l_MarkerW * 0.5f,
              ImDrawFlags_RoundCornersAll);
        }

        ImVec2 l_Cursor(l_BarMin.x + l_PadX, l_BarMin.y + l_PadY);
        const ImVec2 l_ItemSize(l_Icon, l_Icon);
        ImGui::PushID("ViewportUiToolbar");

        for (const ToolbarItem &i_Item : p_Items) {
          ImGui::PushID((int)i_Item.id);
          ImGui::SetCursorScreenPos(l_Cursor);
          ImGui::InvisibleButton("btn", l_ItemSize);

          const bool l_Hovered =
              ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly);
          const bool l_Held = ImGui::IsItemActive();

          if (l_Hovered || l_Held) {
            const ImU32 l_Fill = ImGui::GetColorU32(
                l_Held ? ImGuiCol_ButtonActive
                       : ImGuiCol_ButtonHovered);
            l_Draw->AddRectFilled(ImGui::GetItemRectMin(),
                                  ImGui::GetItemRectMax(), l_Fill,
                                  l_Radius * 0.6f,
                                  ImDrawFlags_RoundCornersAll);
          }

          const ImVec2 l_Center(l_Cursor.x + l_ItemSize.x * 0.5f,
                                l_Cursor.y + l_ItemSize.y * 0.5f);
          if (i_Item.icon_texture) {
            l_Draw->AddImage(
                i_Item.icon_texture,
                ImVec2(l_Center.x - l_Icon * 0.5f,
                       l_Center.y - l_Icon * 0.5f),
                ImVec2(l_Center.x + l_Icon * 0.5f,
                       l_Center.y + l_Icon * 0.5f),
                ImVec2(0, 0), ImVec2(1, 1),
                ImGui::GetColorU32(ImGuiCol_Text));
          } else if (i_Item.icon_glyph) {
            ImVec2 l_TextSize =
                ImGui::CalcTextSize(i_Item.icon_glyph);
            const float l_FontSize = ImGui::GetFontSize();
            const float l_TextScale =
                (l_TextSize.y > 0.0f)
                    ? (l_Icon / l_TextSize.y)
                    : 1.0f;
            const ImVec2 l_SizeScaled(l_TextSize.x * l_TextScale,
                                      l_TextSize.y * l_TextScale);
            l_Draw->AddText(
                ImGui::GetFont(), l_FontSize * l_TextScale,
                ImVec2(l_Center.x - l_SizeScaled.x * 0.5f,
                       l_Center.y - l_SizeScaled.y * 0.5f),
                ImGui::GetColorU32(ImGuiCol_Text),
                i_Item.icon_glyph);
          }

          if (l_Hovered && i_Item.label && *i_Item.label) {
            ImGui::BeginTooltip();
            if (i_Item.shortcut_label && *i_Item.shortcut_label) {
              ImGui::Text("%s  (%s)", i_Item.label,
                          i_Item.shortcut_label);
            } else {
              ImGui::TextUnformatted(i_Item.label);
            }
            ImGui::EndTooltip();
          }

          if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            p_State.active_item = i_Item.id;
          }

          l_Cursor.y += l_ItemSize.y + l_Gap;
          ImGui::PopID();
        }

        ImGui::PopID();
        ImGui::PopClipRect();

        if (ImGui::IsWindowFocused(
                ImGuiFocusedFlags_RootAndChildWindows)) {
          for (const ToolbarItem &i_Item : p_Items) {
            if (i_Item.shortcut_key != ImGuiKey_None &&
                ImGui::Shortcut(i_Item.shortcut_key)) {
              p_State.active_item = i_Item.id;
            }
          }
        }
      }

      void render_standard_left_toolbar(
          ImTextureID p_Background,
          const Util::List<ToolbarItem> &p_Items,
          ToolbarState &p_State, const ToolbarStyle &p_StyleIn)
      {
        ToolbarStyle l_Style = p_StyleIn;
        const float l_Scale = resolve_scale(l_Style.dpi_scale);
        const float l_Margin = l_Style.margin * l_Scale;
        const float l_PadY = l_Style.pad_y * l_Scale;
        const float l_Gap = l_Style.item_gap * l_Scale;
        const float l_Icon = l_Style.icon_size * l_Scale;

        const ImVec2 l_SceneRectMin = ImGui::GetItemRectMin();
        const ImVec2 l_SceneRectMax = ImGui::GetItemRectMax();

        ImGuiWindow *l_Window = ImGui::GetCurrentWindow();
        if (!l_Window) {
          return;
        }

        const ImVec2 l_WinPos = l_Window->Pos;
        const ImVec2 l_ContentMin(
            l_WinPos.x + ImGui::GetWindowContentRegionMin().x,
            l_WinPos.y + ImGui::GetWindowContentRegionMin().y);
        const ImVec2 l_ContentMax(
            l_WinPos.x + ImGui::GetWindowContentRegionMax().x,
            l_WinPos.y + ImGui::GetWindowContentRegionMax().y);

        float l_BarH = l_PadY * 2.0f + l_Icon * p_Items.size() +
                       l_Gap * (float)(p_Items.size() - 1);

        ImVec2 l_Position;
        l_Position.x =
            l_ContentMin.x + l_Margin + l_Style.offset_x * l_Scale;
        if (l_Style.center_vertically) {
          l_Position.y =
              l_ContentMin.y +
              ((l_ContentMax.y - l_ContentMin.y) - l_BarH) * 0.5f;
        } else {
          l_Position.y = l_ContentMin.y + l_Margin;
        }

        render_vertical_toolbar_at(p_Background, l_SceneRectMin,
                                   l_SceneRectMax, l_Position,
                                   p_Items, p_State, l_Style);
      }
    } // namespace ViewportUi
  }   // namespace Editor
} // namespace Low
