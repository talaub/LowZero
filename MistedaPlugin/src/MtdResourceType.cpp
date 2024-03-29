#include "MtdResourceType.h"

#include "LowUtilAssert.h"
#include "LowUtilHandle.h"

namespace Mtd {
  namespace ResourceTypeEnumHelper {
    void initialize()
    {
      Low::Util::RTTI::EnumInfo l_EnumInfo;
      l_EnumInfo.name = N(ResourceType);
      l_EnumInfo.enumId = 2;
      l_EnumInfo.entry_name = &_entry_name;
      l_EnumInfo.entry_value = &_entry_value;

      {
        Low::Util::RTTI::EnumEntryInfo l_Entry;
        l_Entry.name = N(Energy);
        l_Entry.value = 0;

        l_EnumInfo.entries.push_back(l_Entry);
      }
      {
        Low::Util::RTTI::EnumEntryInfo l_Entry;
        l_Entry.name = N(Mana);
        l_Entry.value = 1;

        l_EnumInfo.entries.push_back(l_Entry);
      }

      Low::Util::register_enum_info(2, l_EnumInfo);
    }

    void cleanup()
    {
    }

    Low::Util::Name entry_name(Mtd::ResourceType p_Value)
    {
      if (p_Value == ResourceType::ENERGY) {
        return N(Energy);
      }
      if (p_Value == ResourceType::MANA) {
        return N(Mana);
      }

      LOW_ASSERT(false, "Could not find entry in enum ResourceType.");
      return N(EMPTY);
    }

    Low::Util::Name _entry_name(uint8_t p_Value)
    {
      Mtd::ResourceType l_Enum =
          static_cast<Mtd::ResourceType>(p_Value);
      return entry_name(l_Enum);
    }

    Mtd::ResourceType entry_value(Low::Util::Name p_Name)
    {
      if (p_Name == N(Energy)) {
        return Mtd::ResourceType::ENERGY;
      }
      if (p_Name == N(Mana)) {
        return Mtd::ResourceType::MANA;
      }

      LOW_ASSERT(false, "Could not find entry in enum ResourceType.");
      return static_cast<Mtd::ResourceType>(0);
    }

    uint8_t _entry_value(Low::Util::Name p_Name)
    {
      return static_cast<uint8_t>(entry_value(p_Name));
    }

    u16 get_enum_id()
    {
      return 2;
    }
  } // namespace ResourceTypeEnumHelper
} // namespace Mtd
