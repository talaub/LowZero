#pragma once

#include "LowEditorWidget.h"

#include <imgui_node_editor.h>

namespace Low {
  namespace Editor {
    struct StateGraphWidget : public Widget
    {
      StateGraphWidget();
      void render(float p_Delta) override;

    private:
      ax::NodeEditor::EditorContext *m_Context;
    };
  } // namespace Editor
} // namespace Low
