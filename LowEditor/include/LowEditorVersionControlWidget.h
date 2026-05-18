#pragma once

#include "LowEditorWidget.h"

namespace Low {
  namespace Editor {
    struct VersionControlWidget : public Widget
    {
      void render(float p_Delta) override;
    };

    // Keep the misspelled type as a compatibility alias for any local
    // code that already picked it up.
    using VerionControlWidget = VersionControlWidget;
  } // namespace Editor
} // namespace Low
