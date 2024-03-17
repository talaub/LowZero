#include "MtdResourceType.h"

#include "LowUtilAssert.h"

namespace Mtd {
  namespace ResourceTypeEnumHelper {
    void initialize()
    {
    }

    void cleanup()
    {
    }

    Low::Util::Name option_name(Mtd::ResourceType p_Value)
    {
      if (p_Value == ResourceType::ENERGY) {
        return N(Energy);
      }
      if (p_Value == ResourceType::MANA) {
        return N(Mana);
      }

      LOW_ASSERT(false,
                 "Could not find option in enum ResourceType.");
      return N(EMPTY);
    }

    Low::Util::Name _option_name(uint8_t p_Value)
    {
      Mtd::ResourceType l_Enum =
          static_cast<Mtd::ResourceType>(p_Value);
      return option_name(l_Enum);
    }

    Mtd::ResourceType option_value(Low::Util::Name p_Name)
    {
      if (p_Name == N(Energy)) {
        return Mtd::ResourceType::ENERGY;
      }
      if (p_Name == N(Mana)) {
        return Mtd::ResourceType::MANA;
      }

      LOW_ASSERT(false,
                 "Could not find option in enum ResourceType.");
      return static_cast<Mtd::ResourceType>(0);
    }

    uint8_t _option_value(Low::Util::Name p_Name)
    {
      return static_cast<uint8_t>(option_value(p_Name));
    }
  } // namespace ResourceTypeEnumHelper
} // namespace Mtd
