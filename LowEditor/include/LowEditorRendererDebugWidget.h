#pragma once

#include "LowEditorWidget.h"
#include "LowRendererRenderView.h"

namespace Low {
  namespace Editor {
    struct RendererDebugWidget : public Widget
    {
      void render(float p_Delta) override;

    protected:
      Renderer::RenderView m_RenderView;
    };

  } // namespace Editor
} // namespace Low
