#include "MtdStatusEffectType.h"

#include "LowUtilAssert.h"
#include "LowUtilHandle.h"

namespace Mtd {
  namespace StatusEffectTypeEnumHelper {
    void initialize()
    {
      Low::Util::RTTI::EnumInfo l_EnumInfo;
      l_EnumInfo.name = N(StatusEffectType);
      l_EnumInfo.enumId = 3;
      l_EnumInfo.entry_name = &_entry_name;
      l_EnumInfo.entry_value = &_entry_value;

      {
        Low::Util::RTTI::EnumEntryInfo l_Entry;
        l_Entry.name = N(Buff);
        l_Entry.value = 0;

        l_EnumInfo.entries.push_back(l_Entry);
      }
      {
        Low::Util::RTTI::EnumEntryInfo l_Entry;
        l_Entry.name = N(Debuff);
        l_Entry.value = 1;

        l_EnumInfo.entries.push_back(l_Entry);
      }

      Low::Util::register_enum_info(3, l_EnumInfo);
    }

    void cleanup()
    {
    }

    Low::Util::Name entry_name(Mtd::StatusEffectType p_Value)
    {
      if (p_Value == StatusEffectType::BUFF) {
        return N(Buff);
      }
      if (p_Value == StatusEffectType::DEBUFF) {
        return N(Debuff);
      }

      LOW_ASSERT(false,
                 "Could not find entry in enum StatusEffectType.");
      return N(EMPTY);
    }

    Low::Util::Name _entry_name(uint8_t p_Value)
    {
      Mtd::StatusEffectType l_Enum =
          static_cast<Mtd::StatusEffectType>(p_Value);
      return entry_name(l_Enum);
    }

    Mtd::StatusEffectType entry_value(Low::Util::Name p_Name)
    {
      if (p_Name == N(Buff)) {
        return Mtd::StatusEffectType::BUFF;
      }
      if (p_Name == N(Debuff)) {
        return Mtd::StatusEffectType::DEBUFF;
      }

      LOW_ASSERT(false,
                 "Could not find entry in enum StatusEffectType.");
      return static_cast<Mtd::StatusEffectType>(0);
    }

    uint8_t _entry_value(Low::Util::Name p_Name)
    {
      return static_cast<uint8_t>(entry_value(p_Name));
    }

    u16 get_enum_id()
    {
      return 3;
    }
  } // namespace StatusEffectTypeEnumHelper
} // namespace Mtd
