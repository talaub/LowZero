#pragma once

#include "LowRendererRenderFlow.h"
#include "LowRendererImGuiImage.h"

#include "LowUtilContainers.h"

namespace Low {
  namespace Editor {
    struct RenderFlowWidget
    {
      RenderFlowWidget(Util::String p_Title, Renderer::RenderFlow p_RenderFlow);
      void render(float p_Delta);

    private:
      Util::String m_Title;
      Renderer::RenderFlow m_RenderFlow;
      Renderer::Interface::ImGuiImage m_ImGuiImage;

      Math::UVector2 m_LastFrameDimensions = {0, 0};
      Math::UVector2 m_LastSavedDimensions = {800, 600};
      float m_SaveDimensionTicker = 0.0f;
    };
  } // namespace Editor
} // namespace Low
