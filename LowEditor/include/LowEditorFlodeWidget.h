#pragma once

#include "LowEditorWidget.h"

#include "FlodeEditor.h"

#include <imgui_node_editor.h>

namespace Low {
  namespace Editor {
    struct FlodeWidget : public Widget
    {
      FlodeWidget();
      void render(float p_Delta) override;

      bool handle_shortcuts(float p_Delta) override;

      Flode::Editor *m_Editor;
    };
  } // namespace Editor
} // namespace Low
