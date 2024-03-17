#pragma once

#include "MtdPluginApi.h"

#include "LowMath.h"
#include "LowUtilName.h"

namespace Mtd {
  enum class StatusEffectType
  {
    BUFF,
    DEBUFF,
  };

  namespace StatusEffectTypeEnumHelper {
    void MISTEDA_API initialize();
    void MISTEDA_API cleanup();

    Low::Util::Name MISTEDA_API
    option_name(Mtd::StatusEffectType p_Value);
    Low::Util::Name MISTEDA_API _option_name(uint8_t p_Value);

    Mtd::StatusEffectType MISTEDA_API
    option_value(Low::Util::Name p_Name);
    uint8_t MISTEDA_API _option_value(Low::Util::Name p_Name);
  } // namespace StatusEffectTypeEnumHelper
} // namespace Mtd
