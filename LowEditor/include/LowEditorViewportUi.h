#pragma once

#include "LowEditorApi.h"

#include "LowCoreTween.h"
#include "LowUtilContainers.h"

#include "imgui.h"

namespace Low {
  namespace Editor {
    namespace ViewportUi {
      struct LOW_EDITOR_API ToolbarItem
      {
        u32 id = 0u;
        const char *label = nullptr;
        const char *icon_glyph = nullptr;
        ImTextureID icon_texture = 0;
        const char *shortcut_label = nullptr;
        ImGuiKey shortcut_key = ImGuiKey_None;
      };

      struct LOW_EDITOR_API ToolbarStyle
      {
        float dpi_scale = 1.0f;
        float margin = 23.0f;
        float pad_x = 8.0f;
        float pad_y = 8.0f;
        float item_gap = 12.0f;
        float corner_radius = 10.0f;
        float icon_size = 23.0f;
        float outline_thickness = 1.0f;
        float offset_x = 0.0f;
        bool center_vertically = true;
      };

      struct LOW_EDITOR_API FrostedRectStyle
      {
        float radius = 10.0f;
        ImU32 tint = IM_COL32(255, 255, 255, 10);
        ImU32 border = IM_COL32(0, 0, 0, 0);
        float border_thickness = 1.0f;
      };

      struct LOW_EDITOR_API IconButtonStyle
      {
        float dpi_scale = 1.0f;
        float button_size = 34.0f;
        float corner_radius = 10.0f;
        float icon_size = 18.0f;
      };

      struct LOW_EDITOR_API ButtonToolbarStyle
      {
        float dpi_scale = 1.0f;
        float margin = 23.0f;
        float pad_x = 8.0f;
        float pad_y = 8.0f;
        float item_gap = 12.0f;
        float corner_radius = 10.0f;
        float icon_size = 23.0f;
        float offset_y = 0.0f;
      };

      struct LOW_EDITOR_API ToolbarState
      {
        u32 active_item = 0u;
        bool marker_initialized = false;
        u32 previous_active_item = 0u;
        float marker_start_y = 0.0f;
        float marker_target_y = 0.0f;
        float marker_y = 0.0f;
        Low::Core::Tween marker_tween;
      };

      void LOW_EDITOR_API
      draw_frosted_rect(ImDrawList *p_Draw, ImTextureID p_Background,
                        const ImVec2 &p_RectMin,
                        const ImVec2 &p_RectMax,
                        const ImVec2 &p_SceneRectMin,
                        const ImVec2 &p_SceneRectMax,
                        const FrostedRectStyle &p_Style);

      bool LOW_EDITOR_API
      render_frosted_icon_button_at(
          const char *p_Id, const char *p_Icon,
          const char *p_Tooltip, ImTextureID p_Background,
          const ImVec2 &p_SceneRectMin,
          const ImVec2 &p_SceneRectMax,
          const ImVec2 &p_ButtonCenter,
          const IconButtonStyle &p_Style = IconButtonStyle());

      bool LOW_EDITOR_API render_playmode_button_at(
          ImTextureID p_Background, const ImVec2 &p_SceneRectMin,
          const ImVec2 &p_SceneRectMax,
          const ImVec2 &p_ButtonCenter,
          const IconButtonStyle &p_Style = IconButtonStyle());

      u32 LOW_EDITOR_API render_horizontal_button_toolbar_at(
          ImTextureID p_Background, const ImVec2 &p_SceneRectMin,
          const ImVec2 &p_SceneRectMax, const ImVec2 &p_Position,
          const Util::List<ToolbarItem> &p_Items,
          const ButtonToolbarStyle &p_Style = ButtonToolbarStyle());

      u32 LOW_EDITOR_API render_standard_bottom_button_toolbar(
          ImTextureID p_Background,
          const Util::List<ToolbarItem> &p_Items,
          const ButtonToolbarStyle &p_Style = ButtonToolbarStyle());

      void LOW_EDITOR_API render_vertical_toolbar_at(
          ImTextureID p_Background, const ImVec2 &p_SceneRectMin,
          const ImVec2 &p_SceneRectMax, const ImVec2 &p_Position,
          const Util::List<ToolbarItem> &p_Items,
          ToolbarState &p_State,
          const ToolbarStyle &p_Style = ToolbarStyle());

      void LOW_EDITOR_API render_standard_left_toolbar(
          ImTextureID p_Background,
          const Util::List<ToolbarItem> &p_Items,
          ToolbarState &p_State,
          const ToolbarStyle &p_Style = ToolbarStyle());
    } // namespace ViewportUi
  }   // namespace Editor
} // namespace Low
