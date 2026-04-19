#include "LowEditorNodeGraph.h"
#include "LowEditorGui.h"
#include "LowMath.h"

namespace Low {
  namespace Editor {
    void NodeGraphCanvas::render(const char *p_Label,
                                 const Math::Vector2 &p_Size)
    {
      ImVec2 l_Size = FROM_VEC2(p_Size);

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                          ImVec2(0.0f, 0.0f));

      ImGui::BeginChild(p_Label, l_Size, ImGuiChildFlags_Borders,
                        ImGuiWindowFlags_NoMove |
                            ImGuiWindowFlags_NoScrollWithMouse |
                            ImGuiWindowFlags_NoScrollbar);
      ImGui::PopStyleVar();

      ImDrawList *l_DrawList = ImGui::GetWindowDrawList();

      const ImVec2 l_CanvasP0 = ImGui::GetCursorScreenPos();
      const ImVec2 l_CanvasSize = ImGui::GetContentRegionAvail();
      const ImVec2 l_CanvasP1 = ImVec2(l_CanvasP0.x + l_CanvasSize.x,
                                       l_CanvasP0.y + l_CanvasSize.y);

      // Ensure the child takes up the whole region
      ImGui::InvisibleButton("canvas_area", l_CanvasSize,
                             ImGuiButtonFlags_MouseButtonLeft |
                                 ImGuiButtonFlags_MouseButtonRight |
                                 ImGuiButtonFlags_MouseButtonMiddle);

      const bool l_Hovered = ImGui::IsItemHovered();
      const bool l_Active = ImGui::IsItemActive();

      // Background
      l_DrawList->AddRectFilled(l_CanvasP0, l_CanvasP1,
                                IM_COL32(32, 32, 36, 255));
      l_DrawList->AddRect(l_CanvasP0, l_CanvasP1,
                          IM_COL32(80, 80, 90, 255));

      // Panning with middle mouse
      if (l_Hovered &&
          ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) {
        m_Scrolling.x += ImGui::GetIO().MouseDelta.x;
        m_Scrolling.y += ImGui::GetIO().MouseDelta.y;
      }

      l_DrawList->PushClipRect(l_CanvasP0, l_CanvasP1, true);

      if (m_ShowMinorGrid) {
        draw_grid(l_DrawList, l_CanvasP0, l_CanvasP1, m_MinorGridStep,
                  IM_COL32(45, 45, 50, 255));
      }

      draw_grid(l_DrawList, l_CanvasP0, l_CanvasP1, m_GridStep,
                IM_COL32(60, 60, 68, 255));

      // Origin cross for orientation
      draw_origin(l_DrawList, l_CanvasP0, l_CanvasP1);

      l_DrawList->PopClipRect();

      ImGui::EndChild();
    }

    void NodeGraphCanvas::draw_grid(ImDrawList *p_DrawList,
                                    const ImVec2 &p_Min,
                                    const ImVec2 &p_Max, float p_Step,
                                    ImU32 p_Color) const
    {
      if (p_Step <= 0.0f) {
        return;
      }

      const float l_Width = p_Max.x - p_Min.x;
      const float l_Height = p_Max.y - p_Min.y;

      const float l_OffsetX = std::fmod(m_Scrolling.x, p_Step);
      const float l_OffsetY = std::fmod(m_Scrolling.y, p_Step);

      for (float x = l_OffsetX; x < l_Width; x += p_Step) {
        p_DrawList->AddLine(ImVec2(p_Min.x + x, p_Min.y),
                            ImVec2(p_Min.x + x, p_Max.y), p_Color);
      }

      for (float y = l_OffsetY; y < l_Height; y += p_Step) {
        p_DrawList->AddLine(ImVec2(p_Min.x, p_Min.y + y),
                            ImVec2(p_Max.x, p_Min.y + y), p_Color);
      }
    }

    void NodeGraphCanvas::draw_origin(ImDrawList *p_DrawList,
                                      const ImVec2 &p_Min,
                                      const ImVec2 &p_Max) const
    {
      const ImVec2 l_Origin =
          ImVec2(p_Min.x + m_Scrolling.x, p_Min.y + m_Scrolling.y);

      if (l_Origin.x >= p_Min.x && l_Origin.x <= p_Max.x) {
        p_DrawList->AddLine(ImVec2(l_Origin.x, p_Min.y),
                            ImVec2(l_Origin.x, p_Max.y),
                            IM_COL32(110, 70, 70, 255), 2.0f);
      }

      if (l_Origin.y >= p_Min.y && l_Origin.y <= p_Max.y) {
        p_DrawList->AddLine(ImVec2(p_Min.x, l_Origin.y),
                            ImVec2(p_Max.x, l_Origin.y),
                            IM_COL32(70, 110, 70, 255), 2.0f);
      }
    }
  } // namespace Editor
} // namespace Low
