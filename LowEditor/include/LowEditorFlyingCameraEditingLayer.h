#pragma once

#include "LowEditorApi.h"
#include "LowEditorEditingLayer.h"
#include "LowEditorEditingLayerHelpers.h"

namespace Low {
  namespace Editor {
    struct LOW_EDITOR_API FlyingCameraEditingLayer : public EditingLayer
    {
      FlyingCameraEditingLayer(bool p_BlocksLowerLayers = false,
                               bool p_RequireViewportHover = true);

      void tick(const EditingLayerContext &p_Context) override;
      bool blocks_lower_layers() const override;
      bool tick_camera_controls(const EditingLayerContext &p_Context);

      FlyingCameraControls &get_controls()
      {
        return m_Controls;
      }

      const FlyingCameraControls &get_controls() const
      {
        return m_Controls;
      }

    private:
      FlyingCameraControls m_Controls;
      bool m_BlocksLowerLayers;
      bool m_RequireViewportHover;
    };
  } // namespace Editor
} // namespace Low
