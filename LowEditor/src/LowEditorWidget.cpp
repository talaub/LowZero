#include "LowEditorWidget.h"

#include "imgui.h"

#include "LowEditor.h"

namespace Low {
  namespace Editor {

    void Widget::show(float p_Delta)
    {
      m_Open = true;

      render(p_Delta);

      if (!m_Open) {
        close();
      }
    }

    void Widget::close()
    {
      close_widget(this);
    }
  } // namespace Editor
} // namespace Low
