#include "LowEditorFlyingCameraEditingLayer.h"

#include "LowEditorViewport.h"

namespace Low {
  namespace Editor {
    FlyingCameraEditingLayer::FlyingCameraEditingLayer(
        const bool p_BlocksLowerLayers,
        const bool p_RequireViewportHover)
        : m_BlocksLowerLayers(p_BlocksLowerLayers),
          m_RequireViewportHover(p_RequireViewportHover)
    {
    }

    void FlyingCameraEditingLayer::tick(
        const EditingLayerContext &p_Context)
    {
      tick_camera_controls(p_Context);
    }

    bool FlyingCameraEditingLayer::tick_camera_controls(
        const EditingLayerContext &p_Context)
    {
      if (m_RequireViewportHover && !p_Context.viewport.is_hovered()) {
        return false;
      }

      const bool l_ConsumedMouse =
          m_Controls.tick(p_Context.viewport, p_Context.delta);
      m_Controls.adjust_speed_from_mouse_wheel();

      return l_ConsumedMouse;
    }

    bool FlyingCameraEditingLayer::blocks_lower_layers() const
    {
      return m_BlocksLowerLayers;
    }
  } // namespace Editor
} // namespace Low
