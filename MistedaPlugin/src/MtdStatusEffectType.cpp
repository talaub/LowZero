#include "MtdStatusEffectType.h"

#include "LowUtilAssert.h"

namespace Mtd {
  namespace StatusEffectTypeEnumHelper {
    void initialize()
    {
    }

    void cleanup()
    {
    }

    Low::Util::Name option_name(Mtd::StatusEffectType p_Value)
    {
      if (p_Value == StatusEffectType::BUFF) {
        return N(Buff);
      }
      if (p_Value == StatusEffectType::DEBUFF) {
        return N(Debuff);
      }

      LOW_ASSERT(false,
                 "Could not find option in enum StatusEffectType.");
      return N(EMPTY);
    }

    Low::Util::Name _option_name(uint8_t p_Value)
    {
      Mtd::StatusEffectType l_Enum =
          static_cast<Mtd::StatusEffectType>(p_Value);
      return option_name(l_Enum);
    }

    Mtd::StatusEffectType option_value(Low::Util::Name p_Name)
    {
      if (p_Name == N(Buff)) {
        return Mtd::StatusEffectType::BUFF;
      }
      if (p_Name == N(Debuff)) {
        return Mtd::StatusEffectType::DEBUFF;
      }

      LOW_ASSERT(false,
                 "Could not find option in enum StatusEffectType.");
      return static_cast<Mtd::StatusEffectType>(0);
    }

    uint8_t _option_value(Low::Util::Name p_Name)
    {
      return static_cast<uint8_t>(option_value(p_Name));
    }
  } // namespace StatusEffectTypeEnumHelper
} // namespace Mtd
