#include "LowRendererSkeletonState.h"

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Renderer {
    namespace SkeletonStateEnumHelper {
      void initialize()
      {
        Low::Util::RTTI::EnumInfo l_EnumInfo;
        l_EnumInfo.name = N(SkeletonState);
        l_EnumInfo.enumId = 10;
        l_EnumInfo.entry_name = &_entry_name;
        l_EnumInfo.entry_value = &_entry_value;

        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(Unknown);
          l_Entry.value = 0;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(Unloaded);
          l_Entry.value = 1;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(ScheduledToLoad);
          l_Entry.value = 2;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(LoadingToMemory);
          l_Entry.value = 3;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(Loaded);
          l_Entry.value = 4;

          l_EnumInfo.entries.push_back(l_Entry);
        }

        Low::Util::register_enum_info(10, l_EnumInfo);
      }

      void cleanup()
      {
      }

      Low::Util::Name entry_name(Low::Renderer::SkeletonState p_Value)
      {
        if (p_Value == SkeletonState::UNKNOWN) {
          return N(Unknown);
        }
        if (p_Value == SkeletonState::UNLOADED) {
          return N(Unloaded);
        }
        if (p_Value == SkeletonState::SCHEDULEDTOLOAD) {
          return N(ScheduledToLoad);
        }
        if (p_Value == SkeletonState::LOADINGTOMEMORY) {
          return N(LoadingToMemory);
        }
        if (p_Value == SkeletonState::LOADED) {
          return N(Loaded);
        }

        LOW_ASSERT(false,
                   "Could not find entry in enum SkeletonState.");
        return N(EMPTY);
      }

      Low::Util::Name _entry_name(uint8_t p_Value)
      {
        Low::Renderer::SkeletonState l_Enum =
            static_cast<Low::Renderer::SkeletonState>(p_Value);
        return entry_name(l_Enum);
      }

      Low::Renderer::SkeletonState entry_value(Low::Util::Name p_Name)
      {
        if (p_Name == N(Unknown)) {
          return Low::Renderer::SkeletonState::UNKNOWN;
        }
        if (p_Name == N(Unloaded)) {
          return Low::Renderer::SkeletonState::UNLOADED;
        }
        if (p_Name == N(ScheduledToLoad)) {
          return Low::Renderer::SkeletonState::SCHEDULEDTOLOAD;
        }
        if (p_Name == N(LoadingToMemory)) {
          return Low::Renderer::SkeletonState::LOADINGTOMEMORY;
        }
        if (p_Name == N(Loaded)) {
          return Low::Renderer::SkeletonState::LOADED;
        }

        LOW_ASSERT(false,
                   "Could not find entry in enum SkeletonState.");
        return static_cast<Low::Renderer::SkeletonState>(0);
      }

      uint8_t _entry_value(Low::Util::Name p_Name)
      {
        return static_cast<uint8_t>(entry_value(p_Name));
      }

      u16 get_enum_id()
      {
        return 10;
      }

      u8 get_entry_count()
      {
        return 5;
      }
    } // namespace SkeletonStateEnumHelper
  } // namespace Renderer
} // namespace Low
