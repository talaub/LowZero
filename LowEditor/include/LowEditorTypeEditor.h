#pragma once

#include "LowEditorApi.h"

#include "LowEditorMetadata.h"

#include "LowUtilHandle.h"

#define TYPE_MANAGER_EVENT(eventname)                                \
  virtual void eventname(Util::Handle p_Handle,                      \
                         TypeMetadata &p_Metadata)                   \
  {                                                                  \
  }                                                                  \
  static void handle_##eventname##(Util::Handle p_Handle,            \
                                   TypeMetadata & p_Metadata);       \
  static void handle_##eventname##(Util::Handle p_Handle);

namespace Low {
  namespace Editor {
    struct LOW_EDITOR_API TypeEditor
    {
      virtual void render(Util::Handle p_Handle,
                          TypeMetadata &p_Metadata);

      static void show(Util::Handle p_Handle);
      static void show(Util::Handle p_Handle,
                       TypeMetadata &p_Metadata);

      TYPE_MANAGER_EVENT(after_add)
      TYPE_MANAGER_EVENT(before_delete)
      TYPE_MANAGER_EVENT(after_save)
      TYPE_MANAGER_EVENT(before_save)

      static void register_for_type(u16 p_TypeId,
                                    TypeEditor *p_Editor);

      void show_editor(Util::Handle p_Handle,
                       TypeMetadata &p_Metadata,
                       Util::Name p_PropertyName);

      void show_editor(Util::Handle p_Handle,
                       Util::Name p_PropertyName);

      static void cleanup_registered_types();
    };
  } // namespace Editor
} // namespace Low

#undef TYPE_MANAGER_EVENT
