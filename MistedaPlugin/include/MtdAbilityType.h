#pragma once

#include "MtdPluginApi.h"

#include "LowMath.h"
#include "LowUtilName.h"

namespace Mtd {
  enum class AbilityType : u8
  {
    ATTACK,
    DEFENSE,
    BUFF,
    DEBUFF,
    MAGIC,
  };

  namespace AbilityTypeEnumHelper {
    void MISTEDA_API initialize();
    void MISTEDA_API cleanup();

    Low::Util::Name MISTEDA_API entry_name(Mtd::AbilityType p_Value);
    Low::Util::Name MISTEDA_API _entry_name(uint8_t p_Value);

    Mtd::AbilityType MISTEDA_API entry_value(Low::Util::Name p_Name);
    uint8_t MISTEDA_API _entry_value(Low::Util::Name p_Name);

    u16 MISTEDA_API get_enum_id();
  } // namespace AbilityTypeEnumHelper
} // namespace Mtd
