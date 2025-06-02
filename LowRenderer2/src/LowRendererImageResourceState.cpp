#include "LowRendererImageResourceState.h"

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Renderer {
    namespace ImageResourceStateEnumHelper {
      void initialize()
      {
        Low::Util::RTTI::EnumInfo l_EnumInfo;
        l_EnumInfo.name = N(ImageResourceState);
        l_EnumInfo.enumId = 2;
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
          l_Entry.name = N(MemoryLoaded);
          l_Entry.value = 4;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(UploadingToGpu);
          l_Entry.value = 5;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(Loaded);
          l_Entry.value = 6;

          l_EnumInfo.entries.push_back(l_Entry);
        }

        Low::Util::register_enum_info(2, l_EnumInfo);
      }

      void cleanup()
      {
      }

      Low::Util::Name
      entry_name(Low::Renderer::ImageResourceState p_Value)
      {
        if (p_Value == ImageResourceState::UNKNOWN) {
          return N(Unknown);
        }
        if (p_Value == ImageResourceState::UNLOADED) {
          return N(Unloaded);
        }
        if (p_Value == ImageResourceState::SCHEDULEDTOLOAD) {
          return N(ScheduledToLoad);
        }
        if (p_Value == ImageResourceState::LOADINGTOMEMORY) {
          return N(LoadingToMemory);
        }
        if (p_Value == ImageResourceState::MEMORYLOADED) {
          return N(MemoryLoaded);
        }
        if (p_Value == ImageResourceState::UPLOADINGTOGPU) {
          return N(UploadingToGpu);
        }
        if (p_Value == ImageResourceState::LOADED) {
          return N(Loaded);
        }

        LOW_ASSERT(
            false,
            "Could not find entry in enum ImageResourceState.");
        return N(EMPTY);
      }

      Low::Util::Name _entry_name(uint8_t p_Value)
      {
        Low::Renderer::ImageResourceState l_Enum =
            static_cast<Low::Renderer::ImageResourceState>(p_Value);
        return entry_name(l_Enum);
      }

      Low::Renderer::ImageResourceState
      entry_value(Low::Util::Name p_Name)
      {
        if (p_Name == N(Unknown)) {
          return Low::Renderer::ImageResourceState::UNKNOWN;
        }
        if (p_Name == N(Unloaded)) {
          return Low::Renderer::ImageResourceState::UNLOADED;
        }
        if (p_Name == N(ScheduledToLoad)) {
          return Low::Renderer::ImageResourceState::SCHEDULEDTOLOAD;
        }
        if (p_Name == N(LoadingToMemory)) {
          return Low::Renderer::ImageResourceState::LOADINGTOMEMORY;
        }
        if (p_Name == N(MemoryLoaded)) {
          return Low::Renderer::ImageResourceState::MEMORYLOADED;
        }
        if (p_Name == N(UploadingToGpu)) {
          return Low::Renderer::ImageResourceState::UPLOADINGTOGPU;
        }
        if (p_Name == N(Loaded)) {
          return Low::Renderer::ImageResourceState::LOADED;
        }

        LOW_ASSERT(
            false,
            "Could not find entry in enum ImageResourceState.");
        return static_cast<Low::Renderer::ImageResourceState>(0);
      }

      uint8_t _entry_value(Low::Util::Name p_Name)
      {
        return static_cast<uint8_t>(entry_value(p_Name));
      }

      u16 get_enum_id()
      {
        return 2;
      }

      u8 get_entry_count()
      {
        return 7;
      }
    } // namespace ImageResourceStateEnumHelper
  }   // namespace Renderer
} // namespace Low
