#pragma once

#include "LowEditorWidget.h"
#include "LowEditorViewport.h"

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
      ~EditingWidget();
      void render(float p_Delta) override;
      Viewport *m_Viewport;

    private:
      bool m_SnapTranslation;
      bool m_SnapRotation;
      bool m_SnapScale;

      Math::Vector3 m_SnapTranslationAmount;
      Math::Vector3 m_SnapRotationAmount;
      Math::Vector3 m_SnapScaleAmount;
    };
  } // namespace Editor
} // namespace Low
