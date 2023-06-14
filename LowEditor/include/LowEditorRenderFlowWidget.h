#pragma once

#include "LowRendererRenderFlow.h"
#include "LowRendererImGuiImage.h"

#include "LowUtilContainers.h"

namespace Low {
  namespace Editor {
    struct RenderFlowWidget;

    typedef void (*RenderFlowWidgetCallback)(float, RenderFlowWidget &);

    struct RenderFlowWidget
    {
      RenderFlowWidget(Util::String p_Title, Renderer::RenderFlow p_RenderFlow,
                       RenderFlowWidgetCallback p_Callback);

      void render(float p_Delta);

      Renderer::RenderFlow get_renderflow()
      {
        return m_RenderFlow;
      }

      bool is_hovered();
      Math::Vector2 get_relative_hover_position();
      Math::Vector2 get_widget_position();

    private:
      Util::String m_Title;
      Renderer::RenderFlow m_RenderFlow;
      Renderer::Interface::ImGuiImage m_ImGuiImage;

      Math::UVector2 m_LastFrameDimensions = {0, 0};
      Math::UVector2 m_LastSavedDimensions = {800, 600};
      float m_SaveDimensionTicker = 0.0f;
      Math::Vector2 m_HoveredRelativePosition;
      Math::Vector2 m_WidgetPosition;

      RenderFlowWidgetCallback m_Callback;
    };
  } // namespace Editor
} // namespace Low
