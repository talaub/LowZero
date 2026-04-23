#pragma once

#include "LowCoreApi.h"

#include "LowMath.h"
#include "LowUtilName.h"

namespace Low {
  namespace Core {
    namespace Scripting {
      enum class AssetGenerator : u8
      {
        USERAUTHORED,
        VISUALSCRIPT,
      };

      namespace AssetGeneratorEnumHelper {
        void LOW_CORE_API initialize();
        void LOW_CORE_API cleanup();

        Low::Util::Name LOW_CORE_API
        entry_name(Low::Core::Scripting::AssetGenerator p_Value);
        Low::Util::Name LOW_CORE_API _entry_name(uint8_t p_Value);

        Low::Core::Scripting::AssetGenerator LOW_CORE_API
        entry_value(Low::Util::Name p_Name);
        uint8_t LOW_CORE_API _entry_value(Low::Util::Name p_Name);

        u16 LOW_CORE_API get_enum_id();

        u8 LOW_CORE_API get_entry_count();
      } // namespace AssetGeneratorEnumHelper
    } // namespace Scripting
  } // namespace Core
} // namespace Low
