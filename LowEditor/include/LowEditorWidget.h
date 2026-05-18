#pragma once

#include "LowEditorApi.h"

#include "LowUtilLogger.h"

namespace Low {
  namespace Editor {
    struct LOW_EDITOR_API Widget
    {
      virtual void render(float p_Delta)
      {
      }

      void show(float p_Delta);

      virtual bool handle_shortcuts(float p_Delta)
      {
        return false;
      }

      void close();

      bool is_open() const
      {
        return m_Open;
      }

    protected:
      bool m_Open;
      float m_VerticalSplitter;
    };
  } // namespace Editor
} // namespace Low
