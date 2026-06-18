#include "LowEditorEditingLayer.h"

#include "LowCoreTweenEase.h"
#include "LowEditorThemes.h"
#include "LowEditorViewport.h"

#include "LowCore.h"
#include "LowCoreTweenSystem.h"
#include "LowMath.h"

#include "IconsLucide.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace Low {
  namespace Editor {
    static constexpr float UI_VISIBILITY_TWEEN_DURATION = 0.55f;
    static constexpr float UI_VISIBILITY_MAX_DELTA = 1.0f / 30.0f;

    static float resolve_scale(float p_Scale)
    {
      if (p_Scale <= 0.0f) {
        return ImGui::GetIO().FontGlobalScale;
      }
      return p_Scale;
    }

    static ImVec2
    get_slide_offset(ViewportToolbarSlideDirection p_Direction,
                     const float p_Distance)
    {
      switch (p_Direction) {
      case ViewportToolbarSlideDirection::UP:
        return ImVec2(0.0f, -p_Distance);
      case ViewportToolbarSlideDirection::DOWN:
        return ImVec2(0.0f, p_Distance);
      case ViewportToolbarSlideDirection::LEFT:
        return ImVec2(-p_Distance, 0.0f);
      case ViewportToolbarSlideDirection::RIGHT:
        return ImVec2(p_Distance, 0.0f);
      default:
        return ImVec2(0.0f, p_Distance);
      }
    }

    static ImVec2 calculate_toolbar_position(
        ViewportToolbarAnchor p_Anchor,
        const ViewportUi::ButtonToolbarStyle &p_Style,
        const float p_BarWidth, const float p_BarHeight,
        const ImVec2 &p_ContentMin, const ImVec2 &p_ContentMax)
    {
      const float l_Scale = resolve_scale(p_Style.dpi_scale);
      const float l_Margin = p_Style.margin * l_Scale;

      switch (p_Anchor) {
      case ViewportToolbarAnchor::TOP_CENTER:
        return ImVec2(
            p_ContentMin.x +
                ((p_ContentMax.x - p_ContentMin.x) - p_BarWidth) *
                    0.5f,
            p_ContentMin.y + l_Margin);
      case ViewportToolbarAnchor::TOP_RIGHT:
        return ImVec2(p_ContentMax.x - l_Margin - p_BarWidth,
                      p_ContentMin.y + l_Margin);
      case ViewportToolbarAnchor::LEFT_CENTER:
        return ImVec2(
            p_ContentMin.x + l_Margin,
            p_ContentMin.y +
                ((p_ContentMax.y - p_ContentMin.y) - p_BarHeight) *
                    0.5f);
      case ViewportToolbarAnchor::BOTTOM_CENTER:
      default:
        return ImVec2(
            p_ContentMin.x +
                ((p_ContentMax.x - p_ContentMin.x) - p_BarWidth) *
                    0.5f,
            p_ContentMax.y - l_Margin - p_BarHeight);
      }
    }

    static ImVec2 lerp_imvec2(const ImVec2 &p_A, const ImVec2 &p_B,
                              const float p_Factor)
    {
      return ImVec2(Math::Util::lerp(p_A.x, p_B.x, p_Factor),
                    Math::Util::lerp(p_A.y, p_B.y, p_Factor));
    }

    static bool width_changed(const float p_A, const float p_B)
    {
      return ImFabs(p_A - p_B) > 0.5f;
    }

    static float get_visibility_progress(float &p_Time,
                                         const float p_Delta)
    {
      p_Time = Math::Util::clamp(
          p_Time + Math::Util::clamp(p_Delta, 0.0f,
                                     UI_VISIBILITY_MAX_DELTA),
          0.0f, UI_VISIBILITY_TWEEN_DURATION);
      return p_Time / UI_VISIBILITY_TWEEN_DURATION;
    }

    bool ViewportToolbarContext::icon_button(const char *p_Id,
                                             const char *p_Icon,
                                             const char *p_Tooltip)
    {
      item_count++;

      ImDrawList *l_Draw = ImGui::GetWindowDrawList();

      ImGui::PushID(p_Id);
      ImGui::SetCursorScreenPos(cursor);
      ImGui::InvisibleButton("btn", item_size);

      const bool l_Hovered =
          ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly);
      const bool l_Held = ImGui::IsItemActive();
      const bool l_Pressed =
          ImGui::IsItemClicked(ImGuiMouseButton_Left);

      if (l_Hovered || l_Held) {
        const ImU32 l_Fill = ImGui::GetColorU32(
            l_Held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
        l_Draw->AddRectFilled(
            ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), l_Fill,
            radius * 0.6f, ImDrawFlags_RoundCornersAll);
      }

      if (p_Icon) {
        const ImVec2 l_Center(cursor.x + item_size.x * 0.5f,
                              cursor.y + item_size.y * 0.5f);
        ImVec2 l_TextSize = ImGui::CalcTextSize(p_Icon);
        const float l_FontSize = ImGui::GetFontSize();
        const float l_TextScale =
            (l_TextSize.y > 0.0f) ? (icon_size / l_TextSize.y) : 1.0f;
        const ImVec2 l_SizeScaled(l_TextSize.x * l_TextScale,
                                  l_TextSize.y * l_TextScale);
        l_Draw->AddText(ImGui::GetFont(), l_FontSize * l_TextScale,
                        ImVec2(l_Center.x - l_SizeScaled.x * 0.5f,
                               l_Center.y - l_SizeScaled.y * 0.5f),
                        ImGui::GetColorU32(ImGuiCol_Text), p_Icon);
      }

      if (l_Hovered && p_Tooltip && *p_Tooltip) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(p_Tooltip);
        ImGui::EndTooltip();
      }

      cursor.x += item_size.x + gap;
      ImGui::PopID();

      return l_Pressed;
    }

    float ViewportToolbarContext::get_button_toolbar_width(
        const u32 p_ButtonCount) const
    {
      if (p_ButtonCount == 0u) {
        return pad_x * 2.0f;
      }

      return pad_x * 2.0f + item_size.x * (float)p_ButtonCount +
             gap * (float)(p_ButtonCount - 1u);
    }

    void ViewportToolbarContext::active_vertical_toolbar(
        const Util::List<ViewportUi::ToolbarItem> &p_Items,
        ViewportUi::ToolbarState &p_State)
    {
      if (p_Items.empty()) {
        set_desired_height(get_vertical_button_toolbar_height(0u));
        return;
      }

      set_desired_height(
          get_vertical_button_toolbar_height((u32)p_Items.size()));

      float l_TargetMarkerY = cursor.y + item_size.y * 0.5f;
      for (u32 i = 0u; i < p_Items.size(); ++i) {
        if (p_Items[i].id == p_State.active_item) {
          l_TargetMarkerY = cursor.y +
                            (item_size.y + gap) * (float)i +
                            item_size.y * 0.5f;
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
            Core::Tween::start(0.26f, Core::TweenEase::INOUTCUBIC);
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
        p_State.marker_y = Math::Util::lerp(p_State.marker_start_y,
                                            p_State.marker_target_y,
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
        const float l_MarkerW = 2.0f;
        const float l_GlowW = 5.0f;
        const float l_MarkerH = item_size.y * 1.16f;
        const float l_MarkerX = cursor.x - pad_x + 3.0f;
        l_Draw->AddRectFilled(
            ImVec2(l_MarkerX - 1.5f,
                   p_State.marker_y - l_MarkerH * 0.5f),
            ImVec2(l_MarkerX - 1.5f + l_GlowW,
                   p_State.marker_y + l_MarkerH * 0.5f),
            ImGui::GetColorU32(l_GlowColor), l_GlowW * 0.5f,
            ImDrawFlags_RoundCornersAll);
        l_Draw->AddRectFilled(
            ImVec2(l_MarkerX, p_State.marker_y - l_MarkerH * 0.5f),
            ImVec2(l_MarkerX + l_MarkerW,
                   p_State.marker_y + l_MarkerH * 0.5f),
            ImGui::GetColorU32(l_DebugColor), l_MarkerW * 0.5f,
            ImDrawFlags_RoundCornersAll);
      }

      ImVec2 l_Cursor = cursor;
      ImGui::PushID("ViewportToolbarVerticalItems");
      for (const ViewportUi::ToolbarItem &i_Item : p_Items) {
        ImGui::PushID((int)i_Item.id);
        ImGui::SetCursorScreenPos(l_Cursor);
        ImGui::InvisibleButton("btn", item_size);

        const bool l_Hovered =
            ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly);
        const bool l_Held = ImGui::IsItemActive();

        if (l_Hovered || l_Held) {
          const ImU32 l_Fill =
              ImGui::GetColorU32(l_Held ? ImGuiCol_ButtonActive
                                        : ImGuiCol_ButtonHovered);
          l_Draw->AddRectFilled(
              ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
              l_Fill, radius * 0.6f, ImDrawFlags_RoundCornersAll);
        }

        const ImVec2 l_Center(l_Cursor.x + item_size.x * 0.5f,
                              l_Cursor.y + item_size.y * 0.5f);
        if (i_Item.icon_texture) {
          l_Draw->AddImage(i_Item.icon_texture,
                           ImVec2(l_Center.x - icon_size * 0.5f,
                                  l_Center.y - icon_size * 0.5f),
                           ImVec2(l_Center.x + icon_size * 0.5f,
                                  l_Center.y + icon_size * 0.5f),
                           ImVec2(0, 0), ImVec2(1, 1),
                           ImGui::GetColorU32(ImGuiCol_Text));
        } else if (i_Item.icon_glyph) {
          ImVec2 l_TextSize = ImGui::CalcTextSize(i_Item.icon_glyph);
          const float l_FontSize = ImGui::GetFontSize();
          const float l_TextScale = (l_TextSize.y > 0.0f)
                                        ? (icon_size / l_TextSize.y)
                                        : 1.0f;
          const ImVec2 l_SizeScaled(l_TextSize.x * l_TextScale,
                                    l_TextSize.y * l_TextScale);
          l_Draw->AddText(ImGui::GetFont(), l_FontSize * l_TextScale,
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

        l_Cursor.y += item_size.y + gap;
        item_count++;
        ImGui::PopID();
      }
      ImGui::PopID();

      if (ImGui::IsWindowFocused(
              ImGuiFocusedFlags_RootAndChildWindows)) {
        for (const ViewportUi::ToolbarItem &i_Item : p_Items) {
          if (i_Item.shortcut_key != ImGuiKey_None &&
              ImGui::Shortcut(i_Item.shortcut_key)) {
            p_State.active_item = i_Item.id;
          }
        }
      }
    }

    float ViewportToolbarContext::get_vertical_button_toolbar_height(
        const u32 p_ButtonCount) const
    {
      if (p_ButtonCount == 0u) {
        return pad_y * 2.0f;
      }

      return pad_y * 2.0f + item_size.y * (float)p_ButtonCount +
             gap * (float)(p_ButtonCount - 1u);
    }

    void
    ViewportToolbarContext::set_desired_height(const float p_Height)
    {
      m_DesiredHeight = p_Height;
    }

    float ViewportToolbarContext::get_desired_height() const
    {
      return m_DesiredHeight;
    }

    bool ViewportToolbarContext::playmode_button(const char *p_Id)
    {
      if (Core::get_engine_state() == Util::EngineState::EDITING) {
        return icon_button(p_Id, ICON_LC_PLAY, "Play");
      }
      if (Core::get_engine_state() == Util::EngineState::PLAYING) {
        return icon_button(p_Id, ICON_LC_PAUSE, "Stop");
      }
      return false;
    }

    void ViewportToolbar::show(const EditingLayerUiContext &p_Context)
    {
      if (!content) {
        return;
      }

      const bool l_LocalVisible =
          !hide_in_playmode ||
          Core::get_engine_state() != Util::EngineState::PLAYING;
      update_visibility(l_LocalVisible, p_Context.delta);

      const float l_Visibility = p_Context.visibility * visibility;
      if (l_Visibility <= 0.001f) {
        has_interactive_bounds = false;
        return;
      }

      ViewportUi::ButtonToolbarStyle l_Style = style;
      const float l_Scale = resolve_scale(l_Style.dpi_scale);
      const float l_PadX = l_Style.pad_x * l_Scale;
      const float l_PadY = l_Style.pad_y * l_Scale;
      const float l_Gap = l_Style.item_gap * l_Scale;
      const float l_Radius = l_Style.corner_radius * l_Scale;
      const float l_Icon = l_Style.icon_size * l_Scale;

      const ImVec2 l_SceneRectMin = p_Context.scene_rect_min;
      const ImVec2 l_SceneRectMax = p_Context.scene_rect_max;

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

      ImTextureID l_Background = p_Context.viewport.get_render_view()
                                     .get_blurred_image()
                                     .get_gpu()
                                     .get_imgui_texture_id();

      if (!size_initialized) {
        const ImVec2 l_InitialSize(l_PadX * 2.0f + l_Icon,
                                   l_PadY * 2.0f + l_Icon);
        size_initialized = true;
        size = l_InitialSize;
        size_start = l_InitialSize;
        size_target = l_InitialSize;
      }

      if (size_tween.is_alive()) {
        const float l_TweenProgress =
            Core::System::Tween::get_eased_progress(size_tween);
        size = lerp_imvec2(size_start, size_target, l_TweenProgress);
        if (size_tween.is_finished()) {
          size_tween.destroy();
          size = size_target;
        }
      } else {
        size = size_target;
      }

      const float l_BarWidth = size.x;
      const float l_BarHeight = size.y;

      ImVec2 l_Position = calculate_toolbar_position(
          anchor, l_Style, l_BarWidth, l_BarHeight, l_ContentMin,
          l_ContentMax);
      const float l_HideDistance =
          l_Style.margin * l_Scale + l_BarWidth + l_BarHeight;
      const ImVec2 l_Offset = get_slide_offset(
          slide_direction, l_HideDistance * (1.0f - l_Visibility));
      l_Position.x += l_Offset.x;
      l_Position.y += l_Offset.y;

      const ImVec2 l_BarMin = l_Position;
      const ImVec2 l_BarMax(l_Position.x + l_BarWidth,
                            l_Position.y + l_BarHeight);
      interactive_min = l_BarMin;
      interactive_max = l_BarMax;
      has_interactive_bounds = true;

      ViewportUi::FrostedRectStyle l_FrostedStyle;
      l_FrostedStyle.radius = l_Radius;
      ViewportUi::draw_frosted_rect(
          ImGui::GetWindowDrawList(), l_Background, l_BarMin,
          l_BarMax, l_SceneRectMin, l_SceneRectMax, l_FrostedStyle);

      ViewportToolbarContext l_ToolbarContext{
          p_Context.delta,
          p_Context.viewport,
          l_Background,
          l_SceneRectMin,
          l_SceneRectMax,
          ImVec2(l_BarMin.x + l_PadX, l_BarMin.y + l_PadY),
          ImVec2(l_Icon, l_Icon),
          l_PadX,
          l_PadY,
          l_Gap,
          l_Radius,
          l_Icon};

      const ImVec2 l_PreviousCursorPos = l_Window->DC.CursorPos;
      const ImVec2 l_PreviousCursorMaxPos = l_Window->DC.CursorMaxPos;

      ImGui::PushID(id ? id : "ViewportToolbar");
      ImGui::PushClipRect(l_BarMin, l_BarMax, true);
      const float l_DesiredWidth = content(l_ToolbarContext);
      ImGui::PopClipRect();
      ImGui::PopID();

      l_Window->DC.CursorPos = l_PreviousCursorPos;
      l_Window->DC.CursorMaxPos = l_PreviousCursorMaxPos;

      float l_TargetWidth = l_DesiredWidth;
      if (l_TargetWidth <= 0.0f) {
        l_TargetWidth = l_ToolbarContext.get_button_toolbar_width(
            l_ToolbarContext.item_count);
      }

      float l_TargetHeight = l_ToolbarContext.get_desired_height();
      if (l_TargetHeight <= 0.0f) {
        l_TargetHeight = l_PadY * 2.0f + l_Icon;
      }

      const ImVec2 l_TargetSize(l_TargetWidth, l_TargetHeight);
      if (width_changed(size_target.x, l_TargetSize.x) ||
          width_changed(size_target.y, l_TargetSize.y)) {
        if (size_tween.is_alive()) {
          size_tween.destroy();
        }
        size_start = size;
        size_target = l_TargetSize;
        size_tween =
            Core::Tween::start(0.24f, Core::TweenEase::OUTBACK);
      }
    }

    void ViewportToolbar::cleanup()
    {
      has_interactive_bounds = false;
      if (size_tween.is_alive()) {
        size_tween.destroy();
      }
    }

    bool ViewportToolbar::is_hovered(const ImVec2 &p_Position) const
    {
      return has_interactive_bounds &&
             p_Position.x >= interactive_min.x &&
             p_Position.y >= interactive_min.y &&
             p_Position.x < interactive_max.x &&
             p_Position.y < interactive_max.y;
    }

    void ViewportToolbar::update_visibility(const bool p_Visible,
                                            const float p_Delta)
    {
      if (p_Visible != visibility_target) {
        visibility_target = p_Visible;
        visibility_start = visibility;
        visibility_end = p_Visible ? 1.0f : 0.0f;
        visibility_time = 0.0f;
      }

      if (visibility_time < UI_VISIBILITY_TWEEN_DURATION) {
        const float l_TweenProgress = Core::System::Tween::apply_ease(
            visibility_target ? Core::TweenEase::OUTBACK
                              : Core::TweenEase::INCUBIC,
            get_visibility_progress(visibility_time, p_Delta));
        visibility = Math::Util::lerp(
            visibility_start, visibility_end, l_TweenProgress);
        if (visibility_time >= UI_VISIBILITY_TWEEN_DURATION) {
          visibility = visibility_end;
        }
      } else {
        visibility = visibility_target ? 1.0f : 0.0f;
      }
    }

    EditingLayer::~EditingLayer()
    {
      for (ViewportToolbar &i_Toolbar : m_Toolbars) {
        i_Toolbar.cleanup();
      }
    }

    void EditingLayer::show(const EditingLayerContext &p_Context,
                            const bool p_ShouldTick,
                            const bool p_VisibleTarget)
    {
      update_ui_visibility(p_VisibleTarget, p_Context.delta);

      if (p_ShouldTick) {
        tick(p_Context);
      }

      if (p_VisibleTarget || m_UiVisibility > 0.001f) {
        const ImVec2 l_SceneRectMin = ImGui::GetItemRectMin();
        const ImVec2 l_SceneRectMax = ImGui::GetItemRectMax();
        EditingLayerUiContext l_UiContext{
            p_Context.delta, p_Context.viewport, m_UiVisibility,
            l_SceneRectMin, l_SceneRectMax};
        render_ui(l_UiContext);
        handle_ui(l_UiContext);
      }
    }

    void EditingLayer::initialize_hidden()
    {
      m_UiVisibilityTarget = false;
      m_UiVisibility = 0.0f;
      m_UiVisibilityStart = 0.0f;
      m_UiVisibilityEnd = 0.0f;
      m_UiVisibilityTime = UI_VISIBILITY_TWEEN_DURATION;
      m_Closing = false;
    }

    void EditingLayer::request_close()
    {
      m_Closing = true;
      on_close();
    }

    bool EditingLayer::is_closing() const
    {
      return m_Closing;
    }

    bool EditingLayer::can_remove_from_stack() const
    {
      return m_Closing &&
             m_UiVisibilityTime >= UI_VISIBILITY_TWEEN_DURATION &&
             m_UiVisibility <= 0.001f;
    }

    void
    EditingLayer::update_ui_visibility(const bool p_VisibleTarget,
                                       const float p_Delta)
    {
      if (p_VisibleTarget != m_UiVisibilityTarget) {
        m_UiVisibilityTarget = p_VisibleTarget;
        m_UiVisibilityStart = m_UiVisibility;
        m_UiVisibilityEnd = p_VisibleTarget ? 1.0f : 0.0f;
        m_UiVisibilityTime = 0.0f;
      }

      if (m_UiVisibilityTime < UI_VISIBILITY_TWEEN_DURATION) {
        const float l_TweenProgress = Core::System::Tween::apply_ease(
            p_VisibleTarget ? Core::TweenEase::OUTBACK
                            : Core::TweenEase::INCUBIC,
            get_visibility_progress(m_UiVisibilityTime, p_Delta));
        m_UiVisibility = Math::Util::lerp(
            m_UiVisibilityStart, m_UiVisibilityEnd, l_TweenProgress);
        if (m_UiVisibilityTime >= UI_VISIBILITY_TWEEN_DURATION) {
          m_UiVisibility = m_UiVisibilityEnd;
        }
      } else {
        m_UiVisibility = m_UiVisibilityTarget ? 1.0f : 0.0f;
      }
    }

    ViewportToolbar &EditingLayer::add_toolbar(
        const char *p_Id, const ViewportToolbarAnchor p_Anchor,
        const ViewportToolbarSlideDirection p_SlideDirection,
        ViewportToolbarContent p_Content, const bool p_HideInPlaymode)
    {
      ViewportToolbar l_Toolbar;
      l_Toolbar.id = p_Id;
      l_Toolbar.anchor = p_Anchor;
      l_Toolbar.slide_direction = p_SlideDirection;
      l_Toolbar.content = p_Content;
      l_Toolbar.hide_in_playmode = p_HideInPlaymode;
      m_Toolbars.push_back(l_Toolbar);
      return m_Toolbars.back();
    }

    void
    EditingLayer::handle_ui(const EditingLayerUiContext &p_Context)
    {
      for (ViewportToolbar &i_Toolbar : m_Toolbars) {
        i_Toolbar.show(p_Context);
      }
    }

    bool EditingLayer::is_ui_hovered() const
    {
      const ImVec2 l_MousePosition = ImGui::GetMousePos();
      for (const ViewportToolbar &i_Toolbar : m_Toolbars) {
        if (i_Toolbar.is_hovered(l_MousePosition)) {
          return true;
        }
      }
      return false;
    }

    void
    EditingLayerStack::push(Util::SharedPtr<EditingLayer> p_Layer)
    {
      if (!p_Layer) {
        return;
      }

      p_Layer->initialize_hidden();
      m_Layers.push_back(p_Layer);
    }

    Util::SharedPtr<EditingLayer> EditingLayerStack::pop()
    {
      if (m_Layers.empty()) {
        return nullptr;
      }

      for (u32 i = static_cast<u32>(m_Layers.size()); i > 0u; --i) {
        Util::SharedPtr<EditingLayer> i_Layer = m_Layers[i - 1u];
        if (i_Layer && !i_Layer->is_closing()) {
          i_Layer->request_close();
          return i_Layer;
        }
      }

      return nullptr;
    }

    Util::SharedPtr<EditingLayer> EditingLayerStack::top() const
    {
      if (m_Layers.empty()) {
        return nullptr;
      }

      for (u32 i = static_cast<u32>(m_Layers.size()); i > 0u; --i) {
        Util::SharedPtr<EditingLayer> i_Layer = m_Layers[i - 1u];
        if (i_Layer && !i_Layer->is_closing()) {
          return i_Layer;
        }
      }

      return nullptr;
    }

    void EditingLayerStack::clear()
    {
      m_Layers.clear();
    }

    bool EditingLayerStack::empty() const
    {
      return m_Layers.empty();
    }

    u32 EditingLayerStack::size() const
    {
      return static_cast<u32>(m_Layers.size());
    }

    void EditingLayerStack::tick(const EditingLayerContext &p_Context)
    {
      if (m_Layers.empty()) {
        return;
      }

      u32 l_FirstLayer = 0u;
      for (u32 i = static_cast<u32>(m_Layers.size()); i > 0u; --i) {
        const u32 i_Index = i - 1u;
        if (m_Layers[i_Index] && !m_Layers[i_Index]->is_closing() &&
            m_Layers[i_Index]->blocks_lower_layers()) {
          l_FirstLayer = i_Index;
          break;
        }
      }

      for (u32 i = 0u; i < m_Layers.size(); ++i) {
        Util::SharedPtr<EditingLayer> i_Layer = m_Layers[i];
        if (i_Layer) {
          const bool i_Visible =
              !i_Layer->is_closing() && i >= l_FirstLayer;
          i_Layer->show(p_Context, i_Visible, i_Visible);
        }
      }

      for (u32 i = static_cast<u32>(m_Layers.size()); i > 0u; --i) {
        const u32 i_Index = i - 1u;
        Util::SharedPtr<EditingLayer> i_Layer = m_Layers[i_Index];
        if (i_Layer && i_Layer->can_remove_from_stack()) {
          m_Layers.erase(m_Layers.begin() + i_Index);
        }
      }
    }
  } // namespace Editor
} // namespace Low
