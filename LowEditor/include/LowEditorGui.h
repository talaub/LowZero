#pragma once

#include "LowEditorApi.h"

#include "LowEditorWidget.h"

#include <imgui.h>

#define LOW_EDITOR_SPACING 8.0f
#define LOW_EDITOR_LABEL_WIDTH_REL 0.27f
#define LOW_EDITOR_LABEL_HEIGHT_ABS 20.0f

namespace Low {
  namespace Editor {
    namespace Gui {
      bool Vector3Edit(Math::Vector3 &p_Vector,
                       float p_MaxWidth = -1.0f);
      Util::String FileExplorer();

      bool spinner(const char *label, float radius, int thickness,
                   Math::Color p_Color);

      void drag_handle(Util::Handle p_Handle);

      bool LOW_EDITOR_API SearchField(
          Util::String p_Label, char *p_SearchString, int p_Length,
          ImVec2 p_IconOffset = ImVec2(0.0f, 0.0f));
    } // namespace Gui
  }   // namespace Editor
} // namespace Low
