#pragma once

#include "LowEditorWidget.h"

namespace Low {
  namespace Editor {
    namespace Gui {
      bool Vector3Edit(Math::Vector3 &p_Vector);
      Util::String FileExplorer();

      bool spinner(const char *label, float radius, int thickness,
                   Math::Color p_Color);
    } // namespace Gui
  }   // namespace Editor
} // namespace Low
