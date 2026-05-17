#pragma once

#include "LowEditorApi.h"

#include "LowCoreEntity.h"
#include "LowCoreRegion.h"

#include "LowUtilString.h"

namespace Low {
  namespace Editor {
    namespace EntityTree {
      struct RenderOptions
      {
        Core::Region regionFilter;
        bool filterToRegion = false;
        bool moveDroppedEntitiesToParentRegion = false;
      };

      LOW_EDITOR_API const char *get_icon(Core::Entity p_Entity);

      LOW_EDITOR_API bool matches_search(
          Core::Entity p_Entity, const Util::String &p_Search);

      LOW_EDITOR_API bool render(Core::Entity p_Entity,
                                 bool *p_OpenedEntryPopup,
                                 const Util::String &p_Search,
                                 const RenderOptions &p_Options =
                                     RenderOptions());

      LOW_EDITOR_API void detach_from_parent_preserve_world(
          Core::Entity p_Entity);
    } // namespace EntityTree
  } // namespace Editor
} // namespace Low
