#pragma once

#include "MtdPluginApi.h"

#include "LowMath.h"
#include "LowUtilName.h"

namespace Mtd {
  enum class AbilityType
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

    Low::Util::Name MISTEDA_API option_name(Mtd::AbilityType p_Value);
    Low::Util::Name MISTEDA_API _option_name(uint8_t p_Value);

    Mtd::AbilityType MISTEDA_API option_value(Low::Util::Name p_Name);
    uint8_t MISTEDA_API _option_value(Low::Util::Name p_Name);
  } // namespace AbilityTypeEnumHelper
} // namespace Mtd
