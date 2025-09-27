#pragma once

#include "LowEditorWidget.h"
#include "LowEditorRenderViewWidget.h"

namespace Low {
  namespace Editor {
    enum class EditorTool
    {
      Select,
      Move,
      Rotate,
      Scale
    };

    struct EditingWidget : public Widget
    {
      EditingWidget();
      void render(float p_Delta) override;
      RenderViewWidget *m_RenderViewWidget;

    private:
      float m_CameraSpeed;
      Math::Vector2 m_LastMousePosition;
      Math::Vector2 m_LastPitchYaw;
      bool camera_movement(float p_Delta);
      void set_camera_rotation(const float p_PitchRadians,
                               const float p_YawRadians);
      void render_editing(float p_Delta);

      bool m_SnapTranslation;
      bool m_SnapRotation;
      bool m_SnapScale;

      Math::Vector3 m_SnapTranslationAmount;
      Math::Vector3 m_SnapRotationAmount;
      Math::Vector3 m_SnapScaleAmount;
    };
  } // namespace Editor
} // namespace Low
