#pragma once

#include "LowRendererRenderView.h"

#include "LowUtilContainers.h"

namespace Low {
  namespace Editor {
    struct RenderViewWidget;

    typedef void (*RenderViewWidgetCallback)(float,
                                             RenderViewWidget &);

    struct RenderViewWidget
    {
      RenderViewWidget(Util::String p_Title,
                       Renderer::RenderView p_RenderView,
                       RenderViewWidgetCallback p_Callback);

      void render(float p_Delta);

      Renderer::RenderView get_renderview()
      {
        return m_RenderView;
      }

      bool is_hovered();
      Math::Vector2 get_relative_hover_position();
      Math::Vector2 get_widget_position();
      Math::UVector2 get_widget_dimensions();

      bool is_focused() const;

    private:
      Util::String m_Title;
      Renderer::RenderView m_RenderView;

      Math::UVector2 m_LastFrameDimensions = {0, 0};
      Math::UVector2 m_LastSavedDimensions = {800, 600};
      float m_SaveDimensionTicker = 0.0f;
      Math::Vector2 m_HoveredRelativePosition;
      Math::Vector2 m_WidgetPosition;

      bool m_IsFocused;

      RenderViewWidgetCallback m_Callback;
    };
  } // namespace Editor
} // namespace Low
