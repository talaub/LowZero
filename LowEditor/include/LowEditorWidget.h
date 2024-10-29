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

    protected:
      bool m_Open;
    };
  } // namespace Editor
} // namespace Low
