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
      Util::Handle m_Handle;
      TypeMetadata m_Metadata;
      Util::String m_Title;

      TypeEditor *m_Editor;
    };
  } // namespace Editor
} // namespace Low
