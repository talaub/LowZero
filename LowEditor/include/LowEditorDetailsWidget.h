#pragma once

#include "LowEditorWidget.h"

#include "LowUtilHandle.h"
#include "LowEditorHandlePropertiesSection.h"

namespace Low {
  namespace Editor {
    struct DetailsWidget : public Widget
    {
      void render(float p_Delta) override;

      void add_section(const Util::Handle p_Handle);
      void add_section(HandlePropertiesSection p_Section);
      void clear();

    private:
      Util::List<HandlePropertiesSection> m_Sections;
      bool m_BreakRunning = false;
    };
  } // namespace Editor
} // namespace Low
