#pragma once

#include "LowEditorWidget.h"
#include "LowEditorMetadata.h"
#include "LowEditorTypeEditor.h"

#include "LowUtilHandle.h"

namespace Low {
  namespace Editor {
    struct EditWidget : public Widget
    {
      EditWidget(Util::Handle p_Handle);
      ~EditWidget();

      void render(float p_Delta) override;

      bool handle_is_alive() const
      {
        return m_Metadata.typeInfo.is_alive(m_Handle);
      }

    private:
      void render_title_bar_buttons();

      Util::Handle m_Handle;
      TypeMetadata m_Metadata;
      Util::String m_Title;

      TypeEditor *m_Editor;

      bool m_Maximized = false;
      bool m_RestorePending = false;
      Math::Vector2 m_RestorePosition;
      Math::Vector2 m_RestoreSize;
    };
  } // namespace Editor
} // namespace Low
