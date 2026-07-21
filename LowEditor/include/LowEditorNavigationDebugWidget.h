#pragma once

#include "LowEditorWidget.h"
#include "LowUtilString.h"

namespace Low {
  namespace Editor {
    struct NavigationDebugWidget : public Widget
    {
      void render(float p_Delta) override;

    private:
      bool m_BuildAttempted = false;
      bool m_LastBuildSucceeded = false;
      uint32_t m_LastVertexCount = 0u;
      uint32_t m_LastTriangleCount = 0u;
      Low::Util::String m_LastBuildMessage;
    };
  } // namespace Editor
} // namespace Low
