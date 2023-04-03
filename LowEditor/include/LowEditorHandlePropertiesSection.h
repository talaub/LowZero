#pragma once

#include "LowEditorWidget.h"

#include "LowUtilHandle.h"

namespace Low {
  namespace Editor {
    struct HandlePropertiesSection : public Widget
    {
      HandlePropertiesSection(const Util::Handle p_Handle);
      void render(float p_Delta) override;

    private:
      const Util::Handle m_Handle;
      Util::RTTI::TypeInfo m_TypeInfo;
    };
  } // namespace Editor
} // namespace Low
