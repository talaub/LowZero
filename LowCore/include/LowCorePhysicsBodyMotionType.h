#pragma once

#include "LowCoreApi.h"

#include "LowMath.h"
#include "LowUtilName.h"

namespace Low {
  namespace Core {
    namespace Physics {
      enum class BodyMotionType : u8
      {
        STATIC,
        KINEMATIC,
        DYNAMIC,
      };

      namespace BodyMotionTypeEnumHelper {
        void LOW_CORE_API initialize();
        void LOW_CORE_API cleanup();

        Low::Util::Name LOW_CORE_API
        entry_name(Low::Core::Physics::BodyMotionType p_Value);
        Low::Util::Name LOW_CORE_API _entry_name(uint8_t p_Value);

        Low::Core::Physics::BodyMotionType LOW_CORE_API
        entry_value(Low::Util::Name p_Name);
        uint8_t LOW_CORE_API _entry_value(Low::Util::Name p_Name);

        u16 LOW_CORE_API get_enum_id();

        u8 LOW_CORE_API get_entry_count();
      } // namespace BodyMotionTypeEnumHelper
    } // namespace Physics
  } // namespace Core
} // namespace Low
