#pragma once

#include "LowEditorWidget.h"

namespace Low {
  namespace Editor {
    struct LogWidget : public Widget
    {
      void render(float p_Delta) override;
    };
  } // namespace Editor
} // namespace Low
