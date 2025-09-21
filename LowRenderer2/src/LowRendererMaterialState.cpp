#include "LowRendererMaterialState.h"

#include "LowUtil.h"
#include "LowUtilAssert.h"
#include "LowUtilHandle.h"

namespace Low {
  namespace Renderer {
    namespace MaterialStateEnumHelper {
      void initialize()
      {
        Low::Util::RTTI::EnumInfo l_EnumInfo;
        l_EnumInfo.name = N(MaterialState);
        l_EnumInfo.enumId = 7;
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
          l_Entry.name = N(MemoryLoaded);
          l_Entry.value = 1;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(UploadingToGpu);
          l_Entry.value = 2;

          l_EnumInfo.entries.push_back(l_Entry);
        }
        {
          Low::Util::RTTI::EnumEntryInfo l_Entry;
          l_Entry.name = N(Loaded);
          l_Entry.value = 3;

          l_EnumInfo.entries.push_back(l_Entry);
        }

        Low::Util::register_enum_info(7, l_EnumInfo);
      }

      void cleanup()
      {
      }

      Low::Util::Name entry_name(Low::Renderer::MaterialState p_Value)
      {
        if (p_Value == MaterialState::UNKNOWN) {
          return N(Unknown);
        }
        if (p_Value == MaterialState::MEMORYLOADED) {
          return N(MemoryLoaded);
        }
        if (p_Value == MaterialState::UPLOADINGTOGPU) {
          return N(UploadingToGpu);
        }
        if (p_Value == MaterialState::LOADED) {
          return N(Loaded);
        }

        LOW_ASSERT(false,
                   "Could not find entry in enum MaterialState.");
        return N(EMPTY);
      }

      Low::Util::Name _entry_name(uint8_t p_Value)
      {
        Low::Renderer::MaterialState l_Enum =
            static_cast<Low::Renderer::MaterialState>(p_Value);
        return entry_name(l_Enum);
      }

      Low::Renderer::MaterialState entry_value(Low::Util::Name p_Name)
      {
        if (p_Name == N(Unknown)) {
          return Low::Renderer::MaterialState::UNKNOWN;
        }
        if (p_Name == N(MemoryLoaded)) {
          return Low::Renderer::MaterialState::MEMORYLOADED;
        }
        if (p_Name == N(UploadingToGpu)) {
          return Low::Renderer::MaterialState::UPLOADINGTOGPU;
        }
        if (p_Name == N(Loaded)) {
          return Low::Renderer::MaterialState::LOADED;
        }

        LOW_ASSERT(false,
                   "Could not find entry in enum MaterialState.");
        return static_cast<Low::Renderer::MaterialState>(0);
      }

      uint8_t _entry_value(Low::Util::Name p_Name)
      {
        return static_cast<uint8_t>(entry_value(p_Name));
      }

      u16 get_enum_id()
      {
        return 7;
      }

      u8 get_entry_count()
      {
        return 4;
      }
    } // namespace MaterialStateEnumHelper
  } // namespace Renderer
} // namespace Low
