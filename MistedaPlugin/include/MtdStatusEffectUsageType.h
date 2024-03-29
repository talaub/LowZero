#pragma once

#include "MtdPluginApi.h"

#include "LowMath.h"
#include "LowUtilName.h"

namespace Mtd {
  enum class StatusEffectUsageType : u8
  {
    STACK,
    DURATION,
  };

  namespace StatusEffectUsageTypeEnumHelper {
    void MISTEDA_API initialize();
    void MISTEDA_API cleanup();

    Low::Util::Name MISTEDA_API
    entry_name(Mtd::StatusEffectUsageType p_Value);
    Low::Util::Name MISTEDA_API _entry_name(uint8_t p_Value);

    Mtd::StatusEffectUsageType MISTEDA_API
    entry_value(Low::Util::Name p_Name);
    uint8_t MISTEDA_API _entry_value(Low::Util::Name p_Name);

    u16 MISTEDA_API get_enum_id();
  } // namespace StatusEffectUsageTypeEnumHelper
} // namespace Mtd
