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

      bool Checkbox(const char *label, bool *v);

      bool LOW_EDITOR_API DragFloatWithButtons(
          const char *label, float *value, float speed = 1.0f,
          float min = 0.0f, float max = 0.0f,
          const char *format = "%.3f");

      bool LOW_EDITOR_API DragIntWithButtons(const char *label,
                                             int *value,
                                             int speed = 1,
                                             int min = -50000,
                                             int max = 50000);

      bool LOW_EDITOR_API ButtonRounding(const char *p_Text,
                                         u32 p_Flags);
      bool LOW_EDITOR_API ButtonNoRounding(const char *p_Text);

      bool LOW_EDITOR_API CollapsingHeaderButton(
          const char *label, u32 p_Flags, const char *button_label);
    } // namespace Gui
  }   // namespace Editor
} // namespace Low
