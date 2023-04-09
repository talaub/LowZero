#pragma once

#include "LowEditorWidget.h"

#include "LowUtilHandle.h"
#include "LowEditorHandlePropertiesSection.h"

namespace Low {
  namespace Editor {
    struct ProfilerWidget : public Widget
    {
      void render(float p_Delta) override;
    };
  } // namespace Editor
} // namespace Low
