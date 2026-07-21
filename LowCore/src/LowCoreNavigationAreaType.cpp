#include "LowCoreNavigationAreaType.h"

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Core {
    namespace Navigation {
      namespace AreaTypeEnumHelper {
        void initialize()
        {
          Low::Util::RTTI::EnumInfo l_EnumInfo;
          l_EnumInfo.name = N(AreaType);
          l_EnumInfo.enumId = 17;
          l_EnumInfo.entry_name = &_entry_name;
          l_EnumInfo.entry_value = &_entry_value;

          {
            Low::Util::RTTI::EnumEntryInfo l_Entry;
            l_Entry.name = N(Preferred);
            l_Entry.value = 0;

            l_EnumInfo.entries.push_back(l_Entry);
          }
          {
            Low::Util::RTTI::EnumEntryInfo l_Entry;
            l_Entry.name = N(Normal);
            l_Entry.value = 1;

            l_EnumInfo.entries.push_back(l_Entry);
          }
          {
            Low::Util::RTTI::EnumEntryInfo l_Entry;
            l_Entry.name = N(Rough);
            l_Entry.value = 2;

            l_EnumInfo.entries.push_back(l_Entry);
          }
          {
            Low::Util::RTTI::EnumEntryInfo l_Entry;
            l_Entry.name = N(Difficult);
            l_Entry.value = 3;

            l_EnumInfo.entries.push_back(l_Entry);
          }
          {
            Low::Util::RTTI::EnumEntryInfo l_Entry;
            l_Entry.name = N(Blocked);
            l_Entry.value = 4;

            l_EnumInfo.entries.push_back(l_Entry);
          }

          Low::Util::register_enum_info(17, l_EnumInfo);
        }

        void cleanup()
        {
        }

        Low::Util::Name
        entry_name(Low::Core::Navigation::AreaType p_Value)
        {
          if (p_Value == AreaType::PREFERRED) {
            return N(Preferred);
          }
          if (p_Value == AreaType::NORMAL) {
            return N(Normal);
          }
          if (p_Value == AreaType::ROUGH) {
            return N(Rough);
          }
          if (p_Value == AreaType::DIFFICULT) {
            return N(Difficult);
          }
          if (p_Value == AreaType::BLOCKED) {
            return N(Blocked);
          }

          LOW_ASSERT(false, "Could not find entry in enum AreaType.");
          return N(EMPTY);
        }

        Low::Util::Name _entry_name(uint8_t p_Value)
        {
          Low::Core::Navigation::AreaType l_Enum =
              static_cast<Low::Core::Navigation::AreaType>(p_Value);
          return entry_name(l_Enum);
        }

        Low::Core::Navigation::AreaType
        entry_value(Low::Util::Name p_Name)
        {
          if (p_Name == N(Preferred)) {
            return Low::Core::Navigation::AreaType::PREFERRED;
          }
          if (p_Name == N(Normal)) {
            return Low::Core::Navigation::AreaType::NORMAL;
          }
          if (p_Name == N(Rough)) {
            return Low::Core::Navigation::AreaType::ROUGH;
          }
          if (p_Name == N(Difficult)) {
            return Low::Core::Navigation::AreaType::DIFFICULT;
          }
          if (p_Name == N(Blocked)) {
            return Low::Core::Navigation::AreaType::BLOCKED;
          }

          LOW_ASSERT(false, "Could not find entry in enum AreaType.");
          return static_cast<Low::Core::Navigation::AreaType>(0);
        }

        uint8_t _entry_value(Low::Util::Name p_Name)
        {
          return static_cast<uint8_t>(entry_value(p_Name));
        }

        u16 get_enum_id()
        {
          return 17;
        }

        u8 get_entry_count()
        {
          return 5;
        }
      } // namespace AreaTypeEnumHelper
    } // namespace Navigation
  } // namespace Core
} // namespace Low
