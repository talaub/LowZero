#pragma once

#include "LowUtilName.h"
#include "LowUtilHandle.h"
#include "LowUtilContainers.h"

namespace Low {
  namespace Editor {
    struct PropertyMetadata
    {
      Util::Name name;
      bool editor;
      Util::RTTI::PropertyInfo propInfo;
    };

    struct TypeMetadata
    {
      Util::Name name;
      Util::String module;
      uint16_t typeId;
      Util::RTTI::TypeInfo typeInfo;
      Util::List<PropertyMetadata> properties;
    };
  } // namespace Editor
} // namespace Low
