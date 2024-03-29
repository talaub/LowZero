#include "MtdStatusEffectUsageType.h"

#include "LowUtilAssert.h"
#include "LowUtilHandle.h"

namespace Mtd {
  namespace StatusEffectUsageTypeEnumHelper {
    void initialize()
    {
      Low::Util::RTTI::EnumInfo l_EnumInfo;
      l_EnumInfo.name = N(StatusEffectUsageType);
      l_EnumInfo.enumId = 4;
      l_EnumInfo.entry_name = &_entry_name;
      l_EnumInfo.entry_value = &_entry_value;

      {
        Low::Util::RTTI::EnumEntryInfo l_Entry;
        l_Entry.name = N(Stack);
        l_Entry.value = 0;

        l_EnumInfo.entries.push_back(l_Entry);
      }
      {
        Low::Util::RTTI::EnumEntryInfo l_Entry;
        l_Entry.name = N(Duration);
        l_Entry.value = 1;

        l_EnumInfo.entries.push_back(l_Entry);
      }

      Low::Util::register_enum_info(4, l_EnumInfo);
    }

    void cleanup()
    {
    }

    Low::Util::Name entry_name(Mtd::StatusEffectUsageType p_Value)
    {
      if (p_Value == StatusEffectUsageType::STACK) {
        return N(Stack);
      }
      if (p_Value == StatusEffectUsageType::DURATION) {
        return N(Duration);
      }

      LOW_ASSERT(
          false,
          "Could not find entry in enum StatusEffectUsageType.");
      return N(EMPTY);
    }

    Low::Util::Name _entry_name(uint8_t p_Value)
    {
      Mtd::StatusEffectUsageType l_Enum =
          static_cast<Mtd::StatusEffectUsageType>(p_Value);
      return entry_name(l_Enum);
    }

    Mtd::StatusEffectUsageType entry_value(Low::Util::Name p_Name)
    {
      if (p_Name == N(Stack)) {
        return Mtd::StatusEffectUsageType::STACK;
      }
      if (p_Name == N(Duration)) {
        return Mtd::StatusEffectUsageType::DURATION;
      }

      LOW_ASSERT(
          false,
          "Could not find entry in enum StatusEffectUsageType.");
      return static_cast<Mtd::StatusEffectUsageType>(0);
    }

    uint8_t _entry_value(Low::Util::Name p_Name)
    {
      return static_cast<uint8_t>(entry_value(p_Name));
    }

    u16 get_enum_id()
    {
      return 4;
    }
  } // namespace StatusEffectUsageTypeEnumHelper
} // namespace Mtd
