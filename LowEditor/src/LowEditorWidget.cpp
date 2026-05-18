#include "LowEditorWidget.h"

#include "imgui.h"

#include "LowEditor.h"
#include "LowUtilGlobals.h"

namespace Low {
  namespace Editor {

    void Widget::show(float p_Delta)
    {
      static Util::Name l_SplitterName =
          N(LOW_EDITOR_DETAILS_SPLITTER);
      m_Open = true;
      Util::Globals::set(l_SplitterName, m_VerticalSplitter);

      render(p_Delta);

      m_VerticalSplitter = Util::Globals::get(l_SplitterName);

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
