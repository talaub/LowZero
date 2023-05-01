#pragma once

#include "LowEditorWidget.h"

#include "LowUtilHandle.h"

namespace Low {
  namespace Editor {
    namespace PropertyEditors {
      void render_editor(Util::RTTI::PropertyInfo &p_PropertyInfo,
                         const void *p_DataPtr);

      bool render_handle_selector(Util::String p_Label,
                                  Util::RTTI::TypeInfo &p_TypeInfo,
                                  uint64_t *p_HandleId);
      bool render_handle_selector(Util::String p_Label, uint16_t p_Type,
                                  uint64_t *p_HandleId);
      void render_handle_selector(Util::RTTI::PropertyInfo &p_PropertyInfo,
                                  Util::Handle p_Handle);
    } // namespace PropertyEditors
  }   // namespace Editor
} // namespace Low
