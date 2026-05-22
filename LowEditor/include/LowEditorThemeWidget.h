#pragma once

#include "LowEditorWidget.h"

namespace Low {
  namespace Editor {
    struct ThemeWidget : public Widget
    {
      void render(float p_Delta) override;
    };

  } // namespace Editor
} // namespace Low
