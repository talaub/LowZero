#pragma once

#include "LowMath.h"
#include <imgui.h>

namespace Low {
  namespace Editor {
    struct NodeGraphCanvas
    {
      Math::Vector2 m_Scrolling = Math::Vector2(0, 0);
      float m_GridStep = 64.0f;
      float m_MinorGridStep = 16.0f;
      bool m_ShowMinorGrid = true;

      void render(const char *p_Label,
                  const Math::Vector2 &p_Size = Math::Vector2(0, 0));

      Math::Vector2
      screen_to_canvas(const Math::Vector2 &p_ScreenPos,
                       const Math::Vector2 &p_CanvasOrigin) const
      {
        return Math::Vector2(
            p_ScreenPos.x - p_CanvasOrigin.x - m_Scrolling.x,
            p_ScreenPos.y - p_CanvasOrigin.y - m_Scrolling.y);
      }

      Math::Vector2
      canvas_to_screen(const Math::Vector2 &p_CanvasPos,
                       const Math::Vector2 &p_CanvasOrigin) const
      {
        return Math::Vector2(
            p_CanvasOrigin.x + p_CanvasPos.x + m_Scrolling.x,
            p_CanvasOrigin.y + p_CanvasPos.y + m_Scrolling.y);
      }

      ImVec2 screen_to_canvas(const ImVec2 &p_ScreenPos,
                              const ImVec2 &p_CanvasOrigin) const
      {
        return ImVec2(
            p_ScreenPos.x - p_CanvasOrigin.x - m_Scrolling.x,
            p_ScreenPos.y - p_CanvasOrigin.y - m_Scrolling.y);
      }

      ImVec2 canvas_to_screen(const ImVec2 &p_CanvasPos,
                              const ImVec2 &p_CanvasOrigin) const
      {
        return ImVec2(
            p_CanvasOrigin.x + p_CanvasPos.x + m_Scrolling.x,
            p_CanvasOrigin.y + p_CanvasPos.y + m_Scrolling.y);
      }

    private:
      void draw_grid(ImDrawList *p_DrawList, const ImVec2 &p_Min,
                     const ImVec2 &p_Max, float p_Step,
                     ImU32 p_Color) const;
      void draw_origin(ImDrawList *p_DrawList, const ImVec2 &p_Min,
                       const ImVec2 &p_Max) const;
    };
  } // namespace Editor
} // namespace Low
