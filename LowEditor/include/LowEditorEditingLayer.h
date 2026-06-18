#pragma once

#include "LowEditorApi.h"
#include "LowEditorViewportUi.h"
#include "LowCoreTween.h"
#include "LowMath.h"
#include "LowUtilContainers.h"

namespace Low {
  namespace Editor {
    struct Viewport;

    enum class ViewportToolbarAnchor
    {
      TOP_CENTER,
      TOP_RIGHT,
      LEFT_CENTER,
      BOTTOM_CENTER
    };

    enum class ViewportToolbarSlideDirection
    {
      UP,
      DOWN,
      LEFT,
      RIGHT
    };

    struct LOW_EDITOR_API EditingLayerContext
    {
      float delta;
      Viewport &viewport;
    };

    struct LOW_EDITOR_API EditingLayerUiContext
    {
      float delta;
      Viewport &viewport;
      float visibility;
      ImVec2 scene_rect_min;
      ImVec2 scene_rect_max;
    };

    struct LOW_EDITOR_API ViewportToolbarContext
    {
      float delta;
      Viewport &viewport;
      ImTextureID background;
      ImVec2 scene_rect_min;
      ImVec2 scene_rect_max;
      ImVec2 cursor;
      ImVec2 item_size;
      float pad_x;
      float pad_y;
      float gap;
      float radius;
      float icon_size;
      u32 item_count = 0u;

      bool icon_button(const char *p_Id, const char *p_Icon,
                       const char *p_Tooltip);
      bool playmode_button(const char *p_Id = "PlayModeButton");
      void active_vertical_toolbar(
          const Util::List<ViewportUi::ToolbarItem> &p_Items,
          ViewportUi::ToolbarState &p_State);
      float get_button_toolbar_width(u32 p_ButtonCount) const;
      float
      get_vertical_button_toolbar_height(u32 p_ButtonCount) const;
      void set_desired_height(float p_Height);
      float get_desired_height() const;
      float m_DesiredHeight = 0.0f;
    };

    using ViewportToolbarContent =
        Util::Function<float(ViewportToolbarContext &)>;

    struct LOW_EDITOR_API ViewportToolbar
    {
      const char *id = nullptr;
      ViewportToolbarAnchor anchor =
          ViewportToolbarAnchor::BOTTOM_CENTER;
      ViewportToolbarSlideDirection slide_direction =
          ViewportToolbarSlideDirection::DOWN;
      ViewportUi::ButtonToolbarStyle style;
      ViewportToolbarContent content;
      bool hide_in_playmode = true;
      Core::Tween size_tween;
      bool size_initialized = false;
      ImVec2 size;
      ImVec2 size_start;
      ImVec2 size_target;
      bool visibility_target = true;
      float visibility = 1.0f;
      float visibility_start = 1.0f;
      float visibility_end = 1.0f;
      float visibility_time = 0.0f;
      ImVec2 interactive_min = ImVec2(0.0f, 0.0f);
      ImVec2 interactive_max = ImVec2(0.0f, 0.0f);
      bool has_interactive_bounds = false;

      void show(const EditingLayerUiContext &p_Context);
      void cleanup();
      bool is_hovered(const ImVec2 &p_Position) const;

    private:
      void update_visibility(bool p_Visible, float p_Delta);
    };

    struct LOW_EDITOR_API EditingLayer
    {
      virtual ~EditingLayer();

      void show(const EditingLayerContext &p_Context,
                bool p_ShouldTick, bool p_VisibleTarget);
      void initialize_hidden();
      void request_close();
      bool is_closing() const;
      bool can_remove_from_stack() const;

      virtual void tick(const EditingLayerContext &p_Context) = 0;
      virtual void render_ui(const EditingLayerUiContext &p_Context)
      {
      }

      virtual void on_close()
      {
      }

      virtual bool blocks_lower_layers() const
      {
        return false;
      }

      ViewportToolbar &
      add_toolbar(const char *p_Id, ViewportToolbarAnchor p_Anchor,
                  ViewportToolbarSlideDirection p_SlideDirection,
                  ViewportToolbarContent p_Content,
                  bool p_HideInPlaymode = true);

    protected:
      void handle_ui(const EditingLayerUiContext &p_Context);
      bool is_ui_hovered() const;

    private:
      void update_ui_visibility(bool p_VisibleTarget, float p_Delta);

      Util::List<ViewportToolbar> m_Toolbars;
      bool m_UiVisibilityTarget = true;
      float m_UiVisibility = 1.0f;
      float m_UiVisibilityStart = 1.0f;
      float m_UiVisibilityEnd = 1.0f;
      float m_UiVisibilityTime = 0.0f;
      bool m_Closing = false;
    };

    struct LOW_EDITOR_API EditingLayerStack
    {
      void push(Util::SharedPtr<EditingLayer> p_Layer);
      Util::SharedPtr<EditingLayer> pop();
      Util::SharedPtr<EditingLayer> top() const;
      void clear();

      bool empty() const;
      u32 size() const;

      void tick(const EditingLayerContext &p_Context);

      Util::List<Util::SharedPtr<EditingLayer>> &get_layers()
      {
        return m_Layers;
      }

      const Util::List<Util::SharedPtr<EditingLayer>> &
      get_layers() const
      {
        return m_Layers;
      }

    private:
      Util::List<Util::SharedPtr<EditingLayer>> m_Layers;
    };
  } // namespace Editor
} // namespace Low
