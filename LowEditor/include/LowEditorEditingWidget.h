#pragma once

#include "LowEditorWidget.h"
#include "LowEditorRenderFlowWidget.h"

#include "LowRendererExposedObjects.h"

namespace Low {
  namespace Editor {
    struct EditingWidget : public Widget
    {
      EditingWidget();
      void render(float p_Delta) override;
      RenderFlowWidget *m_RenderFlowWidget;

    private:
      float m_CameraSpeed;
      Math::Vector2 m_LastMousePosition;
      Math::Vector2 m_LastPitchYaw;
      bool camera_movement(float p_Delta);
      void set_camera_rotation(const float p_PitchRadians,
                               const float p_YawRadians);
      void render_editing(float p_Delta);
    };
  } // namespace Editor
} // namespace Low
