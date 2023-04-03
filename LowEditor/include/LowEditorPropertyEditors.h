#pragma once

#include "LowEditorWidget.h"

#include "LowUtilHandle.h"

namespace Low {
  namespace Editor {
    namespace PropertyEditors {
      void render_editor(Util::RTTI::PropertyInfo &p_PropertyInfo,
                         const void *p_DataPtr);
    }
  } // namespace Editor
} // namespace Low
