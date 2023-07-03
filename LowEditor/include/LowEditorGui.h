#pragma once

#include "LowEditorWidget.h"

#define LOW_EDITOR_SPACING 8.0f
#define LOW_EDITOR_LABEL_WIDTH_REL 0.27f
#define LOW_EDITOR_LABEL_HEIGHT_ABS 20.0f

namespace Low {
  namespace Editor {
    namespace Gui {
      bool Vector3Edit(Math::Vector3 &p_Vector);
      Util::String FileExplorer();

      bool spinner(const char *label, float radius, int thickness,
                   Math::Color p_Color);

      void drag_handle(Util::Handle p_Handle);
    } // namespace Gui
  }   // namespace Editor
} // namespace Low
