#include "MtdAbilityType.h"

#include "LowUtilAssert.h"
#include "LowUtilHandle.h"

namespace Mtd {
  namespace AbilityTypeEnumHelper {
    void initialize()
    {
      Low::Util::RTTI::EnumInfo l_EnumInfo;
      l_EnumInfo.name = N(AbilityType);
      l_EnumInfo.enumId = 1;
      l_EnumInfo.entry_name = &_entry_name;
      l_EnumInfo.entry_value = &_entry_value;

      {
        Low::Util::RTTI::EnumEntryInfo l_Entry;
        l_Entry.name = N(Attack);
        l_Entry.value = 0;

        l_EnumInfo.entries.push_back(l_Entry);
      }
      {
        Low::Util::RTTI::EnumEntryInfo l_Entry;
        l_Entry.name = N(Defense);
        l_Entry.value = 1;

        l_EnumInfo.entries.push_back(l_Entry);
      }
      {
        Low::Util::RTTI::EnumEntryInfo l_Entry;
        l_Entry.name = N(Buff);
        l_Entry.value = 2;

        l_EnumInfo.entries.push_back(l_Entry);
      }
      {
        Low::Util::RTTI::EnumEntryInfo l_Entry;
        l_Entry.name = N(Debuff);
        l_Entry.value = 3;

        l_EnumInfo.entries.push_back(l_Entry);
      }
      {
        Low::Util::RTTI::EnumEntryInfo l_Entry;
        l_Entry.name = N(Magic);
        l_Entry.value = 4;

        l_EnumInfo.entries.push_back(l_Entry);
      }

      Low::Util::register_enum_info(1, l_EnumInfo);
    }

    void cleanup()
    {
    }

    Low::Util::Name entry_name(Mtd::AbilityType p_Value)
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

      LOW_ASSERT(false, "Could not find entry in enum AbilityType.");
      return N(EMPTY);
    }

    Low::Util::Name _entry_name(uint8_t p_Value)
    {
      Mtd::AbilityType l_Enum =
          static_cast<Mtd::AbilityType>(p_Value);
      return entry_name(l_Enum);
    }

    Mtd::AbilityType entry_value(Low::Util::Name p_Name)
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

      LOW_ASSERT(false, "Could not find entry in enum AbilityType.");
      return static_cast<Mtd::AbilityType>(0);
    }

    uint8_t _entry_value(Low::Util::Name p_Name)
    {
      return static_cast<uint8_t>(entry_value(p_Name));
    }

    u16 get_enum_id()
    {
      return 1;
    }
  } // namespace AbilityTypeEnumHelper
} // namespace Mtd
