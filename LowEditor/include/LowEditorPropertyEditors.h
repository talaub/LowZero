#pragma once

#include "LowEditorWidget.h"

#include "LowUtilHandle.h"

namespace Low {
  namespace Editor {
    namespace PropertyEditors {
      void render_editor(Util::RTTI::PropertyInfo &p_PropertyInfo,
                         const void *p_DataPtr);

      void render_handle_selector(Util::RTTI::PropertyInfo &p_PropertyInfo,
                                  Util::Handle p_Handle);
    } // namespace PropertyEditors
  }   // namespace Editor
} // namespace Low
