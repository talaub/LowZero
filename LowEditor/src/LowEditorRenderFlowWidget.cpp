#include "LowEditorRenderFlowWidget.h"

#include "LowRenderer.h"

#include "LowUtilLogger.h"

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

      Math::Vector2 l_HoverRelativePosition{2.0f, 2.0f};

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
        if (l_ViewportDimensions.x > 0u && l_ViewportDimensions.y > 0u) {
          ImVec2 l_ImGuiMousePosition = ImGui::GetMousePos();
          ImVec2 l_ImGuiCursorPosition = ImGui::GetCursorScreenPos();
          ImVec2 l_WindowMousePos = {
              l_ImGuiMousePosition.x - l_ImGuiCursorPosition.x,
              l_ImGuiMousePosition.y - l_ImGuiCursorPosition.y};

          if (l_WindowMousePos.x > 0.0f && l_WindowMousePos.y > 0.0f &&
              l_WindowMousePos.x < l_ViewportDimensions.x &&
              l_WindowMousePos.y < l_ViewportDimensions.y) {
            l_HoverRelativePosition.x =
                ((float)l_WindowMousePos.x) / ((float)l_ViewportDimensions.x);
            l_HoverRelativePosition.y =
                ((float)l_WindowMousePos.y) / ((float)l_ViewportDimensions.y);
          }
        }
        m_ImGuiImage.render(l_ViewportDimensions);
      }

      ImGui::End();

      m_RenderFlow.get_resources()
          .get_buffer_resource(N(HoverCoordinatesBuffer))
          .set(&l_HoverRelativePosition);

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
