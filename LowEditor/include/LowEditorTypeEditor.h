#pragma once

#include "LowEditorApi.h"

#include "LowEditorMetadata.h"

#include "LowUtilHandle.h"

namespace Low {
  namespace Editor {
    struct LOW_EDITOR_API TypeEditor
    {
      virtual void render(Util::Handle p_Handle,
                          TypeMetadata &p_Metadata);

      virtual void after_add(Util::Handle p_Handle,
                             TypeMetadata &p_Metadata)
      {
      }

      static void show(Util::Handle p_Handle);
      static void show(Util::Handle p_Handle,
                       TypeMetadata &p_Metadata);

      static void handle_after_add(Util::Handle p_Handle,
                                   TypeMetadata &p_Metadata);
      static void handle_after_add(Util::Handle p_Handle);

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
