#pragma once

#include "LowEditorApi.h"

#include "LowEditorWidget.h"
#include "LowEditorMetadata.h"

#include "LowUtilHandle.h"

namespace Low {
  namespace Editor {
    namespace PropertyEditors {
      void LOW_EDITOR_API render_editor(
          PropertyMetadata &p_PropertyMetadata, Util::Handle p_Handle,
          const void *p_DataPtr, bool p_RenderLabel = true);

      bool LOW_EDITOR_API render_color_selector(Util::String p_Label,
                                                Math::Color *p_Color);

      bool LOW_EDITOR_API render_enum_selector(
          u16 p_EnumId, u8 *p_Value, Util::String p_Label,
          bool p_RenderLabel, Util::List<u8> p_FilterList);

      bool LOW_EDITOR_API render_handle_selector(
          Util::String p_Label, Util::RTTI::TypeInfo &p_TypeInfo,
          uint64_t *p_HandleId);
      bool LOW_EDITOR_API
      render_handle_selector(Util::String p_Label, uint16_t p_Type,
                             uint64_t *p_HandleId);

      bool LOW_EDITOR_API render_enum_selector(u16 p_EnumId,
                                               u8 *p_Value,
                                               Util::String p_Label,
                                               bool p_RenderLabel);

      bool LOW_EDITOR_API render_enum_selector(
          PropertyMetadata &p_Metadata, Util::Handle p_Handle);

      void LOW_EDITOR_API
      render_handle_selector(Util::RTTI::PropertyInfo &p_PropertyInfo,
                             Util::Handle p_Handle);

      void LOW_EDITOR_API render_editor(Util::Handle p_Handle,
                                        TypeMetadata &p_Metadata,
                                        Util::Name p_PropertyName);

      void LOW_EDITOR_API render_editor(Util::Handle p_Handle,
                                        Util::Name p_PropertyName);

      void LOW_EDITOR_API render_editor_no_label(
          Util::Handle p_Handle, TypeMetadata &p_Metadata,
          Util::Name p_PropertyName);

      void LOW_EDITOR_API render_editor_no_label(
          Util::Handle p_Handle, Util::Name p_PropertyName);

      void LOW_EDITOR_API render_editor(
          Util::String p_Label, Util::Function<void()> p_Function);

      void LOW_EDITOR_API render_string_editor(Util::String &p_Label,
                                               Util::String &p_String,
                                               bool p_Multiline,
                                               bool p_RenderLabel);
    } // namespace PropertyEditors
  }   // namespace Editor
} // namespace Low
