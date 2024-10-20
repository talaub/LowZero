#include "LowEditorWidget.h"

#include "imgui.h"

#include "LowEditor.h"

namespace Low {
  namespace Editor {

    void Widget::close()
    {
      close_widget(this);
    }
  } // namespace Editor
} // namespace Low
