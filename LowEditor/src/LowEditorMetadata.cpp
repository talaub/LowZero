#include "LowEditorMetadata.h"

#include "LowUtilAssert.h"

namespace Low {
  namespace Editor {
    PropertyMetadata
    TypeMetadata::find_property_by_name(Util::Name p_Name) const
    {
      for (u32 i = 0; i < properties.size(); ++i) {
        if (properties[i].name == p_Name) {
          return properties[i];
        }
      }
      LOW_ASSERT(false,
                 "Could not find property metadata of name on type");
      return properties[0];
    }
  } // namespace Editor
} // namespace Low