#include "MtdAbilityType.h"

#include "LowUtilAssert.h"

namespace Mtd {
  namespace AbilityTypeEnumHelper {
    void initialize()
    {
    }

    void cleanup()
    {
    }

    Low::Util::Name option_name(Mtd::AbilityType p_Value)
    {
      if (p_Value == AbilityType::ATTACK) {
        return N(Attack);
      }
      if (p_Value == AbilityType::DEFENSE) {
        return N(Defense);
      }
      if (p_Value == AbilityType::BUFF) {
        return N(Buff);
      }
      if (p_Value == AbilityType::DEBUFF) {
        return N(Debuff);
      }
      if (p_Value == AbilityType::MAGIC) {
        return N(Magic);
      }

      LOW_ASSERT(false, "Could not find option in enum AbilityType.");
      return N(EMPTY);
    }

    Low::Util::Name _option_name(uint8_t p_Value)
    {
      Mtd::AbilityType l_Enum =
          static_cast<Mtd::AbilityType>(p_Value);
      return option_name(l_Enum);
    }

    Mtd::AbilityType option_value(Low::Util::Name p_Name)
    {
      if (p_Name == N(Attack)) {
        return Mtd::AbilityType::ATTACK;
      }
      if (p_Name == N(Defense)) {
        return Mtd::AbilityType::DEFENSE;
      }
      if (p_Name == N(Buff)) {
        return Mtd::AbilityType::BUFF;
      }
      if (p_Name == N(Debuff)) {
        return Mtd::AbilityType::DEBUFF;
      }
      if (p_Name == N(Magic)) {
        return Mtd::AbilityType::MAGIC;
      }

      LOW_ASSERT(false, "Could not find option in enum AbilityType.");
      return static_cast<Mtd::AbilityType>(0);
    }

    uint8_t _option_value(Low::Util::Name p_Name)
    {
      return static_cast<uint8_t>(option_value(p_Name));
    }
  } // namespace AbilityTypeEnumHelper
} // namespace Mtd
