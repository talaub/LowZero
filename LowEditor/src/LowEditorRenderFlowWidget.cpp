#include "LowEditorRenderFlowWidget.h"

#include "LowRenderer.h"

#include "imgui.h"

namespace Low {
  namespace Editor {
    const float g_UpdateDimensionTimer = 1.8f;

    RenderFlowWidget::RenderFlowWidget(Util::String p_Title,
                                       Renderer::RenderFlow p_RenderFlow)
        : m_RenderFlow(p_RenderFlow), m_Title(p_Title)
    {
      m_ImGuiImage = Renderer::Interface::ImGuiImage::make(
          N(RenderFlowImGuiImage), p_RenderFlow.get_output_image());
    }

    void RenderFlowWidget::render(float p_Delta)
    {
      ImGui::Begin(m_Title.c_str());

      ImVec2 l_ViewportSize = ImGui::GetContentRegionAvail();
      Math::UVector2 l_ViewportDimensions((uint32_t)l_ViewportSize.x,
                                          (uint32_t)l_ViewportSize.y);

      if (!m_ImGuiImage.is_alive()) {
        m_ImGuiImage = Renderer::Interface::ImGuiImage::make(
            N(RenderFlowImGuiImage), m_RenderFlow.get_output_image());
      }

      if (m_LastFrameDimensions == l_ViewportDimensions &&
          l_ViewportDimensions != m_LastSavedDimensions) {

        Renderer::adjust_renderflow_dimensions(m_RenderFlow,
                                               l_ViewportDimensions);
        m_ImGuiImage.destroy();

        m_LastSavedDimensions = l_ViewportDimensions;

      } else if (m_ImGuiImage.is_alive()) {
        m_ImGuiImage.render(l_ViewportDimensions);
      }

      ImGui::End();

      {
        m_SaveDimensionTicker += p_Delta;
        if (m_SaveDimensionTicker > g_UpdateDimensionTimer) {
          m_SaveDimensionTicker = 0.0f;
          m_LastFrameDimensions = l_ViewportDimensions;
        }
      }
    }
  } // namespace Editor
} // namespace Low
