#include "LowCoreNavigationSourceMode.h"

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Core {
    namespace Navigation {
      namespace SourceModeEnumHelper {
        void initialize()
        {
          Low::Util::RTTI::EnumInfo l_EnumInfo;
          l_EnumInfo.name = N(SourceMode);
          l_EnumInfo.enumId = 15;
          l_EnumInfo.entry_name = &_entry_name;
          l_EnumInfo.entry_value = &_entry_value;

          {
            Low::Util::RTTI::EnumEntryInfo l_Entry;
            l_Entry.name = N(Surface);
            l_Entry.value = 0;

            l_EnumInfo.entries.push_back(l_Entry);
          }
          {
            Low::Util::RTTI::EnumEntryInfo l_Entry;
            l_Entry.name = N(Obstacle);
            l_Entry.value = 1;

            l_EnumInfo.entries.push_back(l_Entry);
          }
          {
            Low::Util::RTTI::EnumEntryInfo l_Entry;
            l_Entry.name = N(Modifier);
            l_Entry.value = 2;

            l_EnumInfo.entries.push_back(l_Entry);
          }
          {
            Low::Util::RTTI::EnumEntryInfo l_Entry;
            l_Entry.name = N(Ignore);
            l_Entry.value = 3;

            l_EnumInfo.entries.push_back(l_Entry);
          }

          Low::Util::register_enum_info(15, l_EnumInfo);
        }

        void cleanup()
        {
        }

        Low::Util::Name
        entry_name(Low::Core::Navigation::SourceMode p_Value)
        {
          if (p_Value == SourceMode::SURFACE) {
            return N(Surface);
          }
          if (p_Value == SourceMode::OBSTACLE) {
            return N(Obstacle);
          }
          if (p_Value == SourceMode::MODIFIER) {
            return N(Modifier);
          }
          if (p_Value == SourceMode::IGNORE) {
            return N(Ignore);
          }

          LOW_ASSERT(false,
                     "Could not find entry in enum SourceMode.");
          return N(EMPTY);
        }

        Low::Util::Name _entry_name(uint8_t p_Value)
        {
          Low::Core::Navigation::SourceMode l_Enum =
              static_cast<Low::Core::Navigation::SourceMode>(p_Value);
          return entry_name(l_Enum);
        }

        Low::Core::Navigation::SourceMode
        entry_value(Low::Util::Name p_Name)
        {
          if (p_Name == N(Surface) || p_Name == N(Walkable)) {
            return Low::Core::Navigation::SourceMode::SURFACE;
          }
          if (p_Name == N(Obstacle)) {
            return Low::Core::Navigation::SourceMode::OBSTACLE;
          }
          if (p_Name == N(Modifier)) {
            return Low::Core::Navigation::SourceMode::MODIFIER;
          }
          if (p_Name == N(Ignore)) {
            return Low::Core::Navigation::SourceMode::IGNORE;
          }

          LOW_ASSERT(false,
                     "Could not find entry in enum SourceMode.");
          return static_cast<Low::Core::Navigation::SourceMode>(0);
        }

        uint8_t _entry_value(Low::Util::Name p_Name)
        {
          return static_cast<uint8_t>(entry_value(p_Name));
        }

        u16 get_enum_id()
        {
          return 15;
        }

        u8 get_entry_count()
        {
          return 4;
        }
      } // namespace SourceModeEnumHelper
    } // namespace Navigation
  } // namespace Core
} // namespace Low
