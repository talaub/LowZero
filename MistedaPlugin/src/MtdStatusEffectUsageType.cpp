#include "MtdStatusEffectUsageType.h"

#include "LowUtilAssert.h"

namespace Mtd {
  namespace StatusEffectUsageTypeEnumHelper {
    void initialize()
    {
    }

    void cleanup()
    {
    }

    Low::Util::Name option_name(Mtd::StatusEffectUsageType p_Value)
    {
      if (p_Value == StatusEffectUsageType::STACK) {
        return N(Stack);
      }
      if (p_Value == StatusEffectUsageType::DURATION) {
        return N(Duration);
      }

      LOW_ASSERT(
          false,
          "Could not find option in enum StatusEffectUsageType.");
      return N(EMPTY);
    }

    Low::Util::Name _option_name(uint8_t p_Value)
    {
      Mtd::StatusEffectUsageType l_Enum =
          static_cast<Mtd::StatusEffectUsageType>(p_Value);
      return option_name(l_Enum);
    }

    Mtd::StatusEffectUsageType option_value(Low::Util::Name p_Name)
    {
      if (p_Name == N(Stack)) {
        return Mtd::StatusEffectUsageType::STACK;
      }
      if (p_Name == N(Duration)) {
        return Mtd::StatusEffectUsageType::DURATION;
      }

      LOW_ASSERT(
          false,
          "Could not find option in enum StatusEffectUsageType.");
      return static_cast<Mtd::StatusEffectUsageType>(0);
    }

    uint8_t _option_value(Low::Util::Name p_Name)
    {
      return static_cast<uint8_t>(option_value(p_Name));
    }
  } // namespace StatusEffectUsageTypeEnumHelper
} // namespace Mtd
