#include "LowRendererAnimationClipState.h"

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Renderer {
    namespace AnimationClipStateEnumHelper {
      void initialize()
      {
        Low::Util::RTTI::EnumInfo l_EnumInfo;
        l_EnumInfo.name = N(AnimationClipState);
        l_EnumInfo.enumId = 12;
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

        Low::Util::register_enum_info(12, l_EnumInfo);
      }

      void cleanup()
      {
      }

      Low::Util::Name
      entry_name(Low::Renderer::AnimationClipState p_Value)
      {
        if (p_Value == AnimationClipState::UNKNOWN) {
          return N(Unknown);
        }
        if (p_Value == AnimationClipState::UNLOADED) {
          return N(Unloaded);
        }
        if (p_Value == AnimationClipState::SCHEDULEDTOLOAD) {
          return N(ScheduledToLoad);
        }
        if (p_Value == AnimationClipState::LOADINGTOMEMORY) {
          return N(LoadingToMemory);
        }
        if (p_Value == AnimationClipState::LOADED) {
          return N(Loaded);
        }

        LOW_ASSERT(
            false,
            "Could not find entry in enum AnimationClipState.");
        return N(EMPTY);
      }

      Low::Util::Name _entry_name(uint8_t p_Value)
      {
        Low::Renderer::AnimationClipState l_Enum =
            static_cast<Low::Renderer::AnimationClipState>(p_Value);
        return entry_name(l_Enum);
      }

      Low::Renderer::AnimationClipState
      entry_value(Low::Util::Name p_Name)
      {
        if (p_Name == N(Unknown)) {
          return Low::Renderer::AnimationClipState::UNKNOWN;
        }
        if (p_Name == N(Unloaded)) {
          return Low::Renderer::AnimationClipState::UNLOADED;
        }
        if (p_Name == N(ScheduledToLoad)) {
          return Low::Renderer::AnimationClipState::SCHEDULEDTOLOAD;
        }
        if (p_Name == N(LoadingToMemory)) {
          return Low::Renderer::AnimationClipState::LOADINGTOMEMORY;
        }
        if (p_Name == N(Loaded)) {
          return Low::Renderer::AnimationClipState::LOADED;
        }

        LOW_ASSERT(
            false,
            "Could not find entry in enum AnimationClipState.");
        return static_cast<Low::Renderer::AnimationClipState>(0);
      }

      uint8_t _entry_value(Low::Util::Name p_Name)
      {
        return static_cast<uint8_t>(entry_value(p_Name));
      }

      u16 get_enum_id()
      {
        return 12;
      }

      u8 get_entry_count()
      {
        return 5;
      }
    } // namespace AnimationClipStateEnumHelper
  } // namespace Renderer
} // namespace Low
